#include "server.hpp"
#include "parser.hpp"
#include "parsedMessage.hpp"
#include "commandHandler.hpp"

bool Server::running = true;

// Global pointer used by sendMsg() in commandHandler.cpp to queue outgoing data
// through the Server's POLLOUT-based flush mechanism instead of calling send() directly.
Server* g_server = NULL;

Server::Server(const std::string& port, std::string password)
    : port(port), password(password), sockfd(-1), max_conn(10)
{
    g_server = this;
}

Server::~Server()
{
    stop();
}

bool Server::start()
{
    if (!setup_socket())
        return false;

    if (listen(sockfd, max_conn) < 0)
    {
        perror("listen");
        close(sockfd);
        return false;
    }

    setup_poll();
    run();
    return true;
}

bool Server::setup_socket()
{
    struct addrinfo hints, *p, *res;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port.c_str(), &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return false;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
            continue;

        if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
        {
            close(sockfd);
            continue;
        }

        int yes = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (p == NULL)
    {
        std::cerr << "Failed to bind socket\n";
        return false;
    }

    return true;
}

void Server::setup_poll()
{
    pollfd server_fd;
    server_fd.fd = sockfd;
    server_fd.events = POLLIN;
    server_fd.revents = 0;

    fds.push_back(server_fd);
}

void Server::run()
{
    signal(SIGPIPE, SIG_IGN);   // ignore broken pipe
    signal(SIGINT, Server::signal_handler);   // Ctrl+C
    signal(SIGTERM, Server::signal_handler);  // kill command

    while (running)
    {
        if (fds.empty())
            continue;
        int ready = poll(fds.data(), fds.size(), -1);

        if (ready < 0)
        {
            if (errno == EINTR) // signal interrupt
                break;
            perror("poll");
            break;
        }

        size_t i = 0;
        while (i < fds.size())
        {
            if (fds[i].revents & (POLLERR | POLLHUP))
            {
                disconnect_client(i); // i++ not needed
            }
            else if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == sockfd)
                {
                    accept_client();
                    i++;
                }
                else
                {
                    bool removed = handle_client(i);
                    if (!removed)
                        i++;
                }
            }
            else if (fds[i].revents & POLLOUT)
            {
                flush_client(i);
                i++;
            }
            else
            {
                i++;
            }
        }
    }
}

void Server::accept_client()
{
    int client_fd = accept(sockfd, NULL, NULL);
    if (client_fd < 0)
        return;

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(client_fd);
        return;
    }

    pollfd p;
    p.fd = client_fd;
    p.events = POLLIN;
    p.revents = 0;

    fds.push_back(p);

    // Initialise a Client entry for this fd
    Client newClient;
    newClient.setFd(client_fd);
    newClient.setBuffer("");
    newClient.setNickname("");
    newClient.setNicknameToUp("");
    newClient.setUsername("");
    newClient.setPassOk(false);
    newClient.setRegistered(false);
    clientsByFd[client_fd] = newClient;
    // fdByNickUp is not filled yet: no nickname exists at connection time

    std::cout << "New client: " << client_fd << std::endl; // debug message
}

bool Server::handle_client(size_t i)
{
    int fd = fds[i].fd;

    char buf[1024];
    int bytes = recv(fd, buf, sizeof(buf), 0);

    if (bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false;
        // socket error
        disconnect_client(i);
        return true;
    }

    if (bytes == 0)        // client disconnected
    {
        disconnect_client(i);
        return true;
    }

    client_buffers[fd].incoming.append(buf, bytes);

    std::string& data = client_buffers[fd].incoming;
    size_t pos;

    while ((pos = data.find("\r\n")) != std::string::npos)
    {
        std::string msg = data.substr(0, pos);
        data.erase(0, pos + 2);

        process_message(fd, msg);

        // The client may have been removed by process_message (e.g. QUIT or bad PASS)
        if (clientsByFd.find(fd) == clientsByFd.end())
            return true;
    }
    return false;
}

void Server::process_message(int client_fd, const std::string& msg)
{
    ParsedMessage parsed = parseMessage(msg);

    if (!handleCommand(client_fd, parsed, clientsByFd, fdByNickUp, password, channels))
    {
        std::cout << "Client disconnected!" << std::endl;
        // Find the index of this fd in fds to call disconnect_client properly
        for (size_t i = 0; i < fds.size(); i++)
        {
            if (fds[i].fd == client_fd)
            {
                // Send the closing error before disconnecting
                prepare_send(client_fd, "ERROR :Closing Link\r\n");
                flush_client(i);
                disconnect_client(i);
                return;
            }
        }
    }
}

void Server::prepare_send(int fd, const std::string& msg)
{
    client_buffers[fd].outgoing += msg;
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == fd)
        {
            fds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::flush_client(size_t i)
{
    int fd = fds[i].fd;
    std::string& buf = client_buffers[fd].outgoing;

    int bytes = send(fd, buf.c_str(), buf.size(), 0);

    if (bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnect_client(i);
        return;
    }

    buf.erase(0, bytes);

    if (buf.empty())
        fds[i].events &= ~POLLOUT;
}

void Server::disconnect_client(size_t i)
{
    int fd = fds[i].fd;

    // Notify channel members and clean up channel membership, mirroring removeClient()
    std::map<int, Client>::iterator clientIt = clientsByFd.find(fd);
    if (clientIt != clientsByFd.end())
    {
        Client& client = clientIt->second;
        std::string nick = client.getNickname();
        std::string user = client.getUsername();
        if (user.empty())
            user = "unknown";
        std::string nickUp = client.getNicknameToUp();
        const std::set<std::string>& joinedChans = client.getChannels();

        std::string quitMsg = ":" + nick + "!" + user + "@localhost QUIT :Disconnected";

        // Remove client from every channel they are in
        for (std::set<std::string>::const_iterator cit = joinedChans.begin(); cit != joinedChans.end(); ++cit)
        {
            std::map<std::string, Channel>::iterator chanIt = channels.find(*cit);
            if (chanIt != channels.end())
            {
                Channel& channel = chanIt->second;
                const std::set<int>& users = channel.getUsers();
                for (std::set<int>::const_iterator uIt = users.begin(); uIt != users.end(); ++uIt)
                {
                    if (*uIt != fd)
                        prepare_send(*uIt, quitMsg + "\r\n");
                }
                channel.removeUser(fd);
                channel.removeOperator(fd);

                // Remove channel if empty
                if (channel.getUsers().empty())
                    channels.erase(chanIt);
            }
        }

        // Remove from nick lookup map
        std::map<std::string, int>::iterator nickIt = fdByNickUp.find(nickUp);
        if (nickIt != fdByNickUp.end())
            fdByNickUp.erase(nickIt);

        clientsByFd.erase(clientIt);
    }

    close(fd);
    client_buffers.erase(fd);
    fds.erase(fds.begin() + i);
}

void Server::stop()
{
    for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); it++)
        close(it->fd);

    fds.clear();
    client_buffers.clear();
    clientsByFd.clear();
    fdByNickUp.clear();
    channels.clear();
    sockfd = -1;
}

void Server::signal_handler(int sig)
{
    (void)sig;
    running = false;
}
