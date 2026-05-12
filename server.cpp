#include "server.hpp"
#include "parser.hpp"
#include "parsedMessage.hpp"
#include "commandHandler.hpp"
#include "Channel.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <signal.h>
#include <vector>
#include <map>

bool running = true;

void handleSignal(int signal)
{
    (void)signal;
    running = false;
}

//create server socket
int socketCreation()
{
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        std::cerr << "socket() failed" << std::endl;
        return -1;
    }
    std::cout << "Socket created successfully" << std::endl;
    return serverFd;
}

//make serverFd non-blocking
int fdNonBlocking(int fd)
{
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl() failed" << std::endl;
        close(fd);
        return 0;
    }
    return 1;
}

//make the port immediately reusable
int reusableImmediately(int fd)
{
    int optval = 1; 
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        std::cerr << "setsockopt() failed" << std::endl;
        close(fd);
        return 0;
    }
    return 1;
}

//bind, attach socket to IP + PORT
int bindSocket(int port,  int serverFd)
{
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // I need it because there are fields that I willnot initiallize here and they would contain garbage if I don;t zero them with memset
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "bind() failed" << std::endl;
        close(serverFd);
        return 0;
    }
    std::cout << "Bind successful" << std::endl;
    return 1;
}

//listen
int listenSocket(int serverFd)
{
    if(listen(serverFd,10) == -1)
    {
        std::cerr << "listen() failed" << std::endl;
        close(serverFd);
        return 0;
    }
    std::cout << "Server is listening" << std::endl;
    return 1;
}

int pollSockets( std::vector<pollfd>& fds)
{ 
    int ret = poll(&fds[0], fds.size(), -1);   
    if(ret == -1)
    {
        if(errno == EINTR)
        {
            return 2; // -> continue;
        }
        std::cerr << "poll() failed" << std::endl;
        //close(serverFd); // no needed because out of the pollLoop I close everything
        return 3; // return 1;
    }
    return 1;
}

//create vector for fd structures and add the stucture of the serverFd (after listen() because the socket must be ready-to-use)
void createFdPollStrct(int serverFd, pollfd& serverStrctFd)
{    
    serverStrctFd.fd = serverFd;
    serverStrctFd.events = POLLIN;
    serverStrctFd.revents = 0;
}
 
int pollLoop(int serverFd, std::vector<pollfd>& fds, std::map<int, Client>& clientsByFd, 
            std::map<std::string, int> fdByNickUp, const std::string& pass, std::map<std::string, Channel>& channels)
{
    while(running)
    {
        int clientFd;        
        pollfd clientStrctFd;
        std::vector<pollfd> newFds;
        int status = pollSockets(fds);
        if(status == 2)
        {
            continue;
        }
        else if (status == 3)
        {
            return 1;
        }
        std::vector<pollfd>::iterator it;
        
        for(it = fds.begin(); it != fds.end(); ++it)
        {
            if(it->revents & POLLIN)
            {
                if(it->fd == serverFd) //SERVER FD ACTION
                {
                    //client tries to connect
                    while(1)
                    {                        
                        clientFd = accept(serverFd, NULL, NULL);
                        if (clientFd == -1) //accept() failed
                        {
                            if((errno == EWOULDBLOCK) || (errno == EAGAIN))
                            {
                                break;
                            }
                            std::cerr << "accept() failed" << std::endl;
                            break;
                        }
                        else // accept() success
                        {
                            //make clientFd non-blocking
                            if(!fdNonBlocking(clientFd))
                            {
                                continue;
                            }
                            //for the pollfd struct I don't need memset to zero fields because I do fill them all manually:
                            clientStrctFd.fd = clientFd;
                            clientStrctFd.events = POLLIN;
                            clientStrctFd.revents = 0;          
                            Client newClient;
                            newClient.setFd(clientFd);
                            newClient.setBuffer("");
                            newClient.setNickname("");
                            newClient.setNicknameToUp("");
                            newClient.setUsername("");
                            newClient.setPassOk(false);
                            newClient.setRegistered(false);
                            clientsByFd[clientFd] = newClient;
                            //I don't need to add it to fdByNickUp map because no nickname exists yet
                            
                            newFds.push_back(clientStrctFd);
                            std::cout << "Client connected!" << std::endl;
                        }                        
                    }
                }
                else //CLIENT FD ACTION : it->fd
                {
                    char recvbuff[1024];
                    ssize_t result = recv(it->fd, recvbuff, 1023, 0);
                    if ((result < 0))
                    {
                        if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                        {
                            continue;
                        }
                        std::cerr << "recv() failed!" << std::endl;
                        removeClient(it, clientsByFd, fds, fdByNickUp);
                        break;
                    }
                    else if(result == 0)
                    {
                        std::cout << "Client disconnected!" << std::endl;
                        removeClient(it, clientsByFd, fds, fdByNickUp);
                        break;
                    }
                    int fd = it->fd;
                    clientsByFd[fd].appendToBuffer(recvbuff, result); // append received data (may contain partial or multiple messages)
                    size_t nlPos = 0;
                    while (1) // process all complete lines ending with '\n'
                    {
                        nlPos = clientsByFd[fd].getBuffer().find('\n', 0); //find position of first '\n' in the buffer (end of a complete IRC line)
                        if(nlPos != std::string::npos) // if you find it
                        {
                            std::string line = clientsByFd[fd].getBuffer().substr(0, nlPos); //extract one complete line (without '\n')
                            if (!line.empty() && line[line.size() - 1] == '\r') //if the line is finishing with '\r'
                                line.erase(line.size() - 1);                    //then remove the '\r' also
                            clientsByFd[fd].eraseFromBuffer(0, nlPos+1); //and remove this line from the clients buffer (because the buffer should keep only the incomplete lines)

                            //fill struct ParsedMessage with COMMANDS and ARGUMENTS
                            ParsedMessage parsed = parseMessage(line);
                            
                            //call the command handler
                            if (!handleCommand(fd, parsed, clientsByFd, fdByNickUp, pass, channels))
                            {
                                std::cout << "Client disconnected!" << std::endl;
                                removeClient(it, clientsByFd, fds, fdByNickUp); //!!!I get segmantation fault
                                break;
                            }

                        }
                        else //if you don't find any '\n' break out of the loop leaving the clients buffer with  any remaining incomplete data and check in the for loop for the next fd if it has any event 
                        {
                            break;
                        }
                    }
                }
            }
        }
        
        if (!newFds.empty())
        {
            fds.insert(fds.end(), newFds.begin(), newFds.end());// always push_back outside of a for loop that uses iterator to access the elements
        }        
    }
    return 0;
}