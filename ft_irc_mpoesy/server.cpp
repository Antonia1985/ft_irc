#include "server.hpp"

bool Server::running = true;

Server::Server(const std::string& port, std::string password)
    : port(port), password(password), sockfd(-1), max_conn(10)
{
    //running = true;
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

    std::cout << "New client: " << client_fd << std::endl; // debug message
}

// TODO: maybe change that
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
    }
    return false;
}

// TODO: actual function
void Server::process_message(int client_fd, const std::string& msg)
{
    std::cout << "Client " << client_fd << " says: " << msg << std::endl;
    prepare_send(client_fd, "You said: " + msg + "\r\n");
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
    sockfd = -1;
}

void Server::signal_handler(int sig)
{
    (void)sig;
    running = false;
}
