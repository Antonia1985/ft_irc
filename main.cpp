#include "parser.hpp"
#include "client.hpp"
#include "server.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h> //need it here for close()
#include <vector>
#include <cerrno>
#include <signal.h> // need it here for signal();


void handleSignal(int signal);

int main(int ac, char** av)
{
    if(ac != 3 )
    {
        std::cout << "Usage: ./ircserv <port> <password>" << std:: endl;
        return 1;
    }

    int port = std::atoi(av[1]); //if port = abc then atoi("abc") = 0
    if (port <= 0 || port > 65535)
    {
        std::cerr << "The port number is not correct" << std:: endl;
        return 1;
    }

    std::string pass = av[2];
    std::cout << "Starting server on port " << port << " with password " << pass << std::endl;

    //create server socket
    int serverFd = socketCreation();
    if(serverFd == -1)
    {
        return 1;
    }

    //make serverFd non-blocking
    if (!fdNonBlocking(serverFd))
    {
        return 1;
    }    

    //make the port immediately reusable
    if (!reusableImmediately(serverFd))
    {
        return 1;
    }   
    
    //bind, attach socket to IP + PORT
    if(!bindSocket(port, serverFd))
    {
        return 1;
    }

    //listen
    if(!listenSocket(serverFd))
    {
        return 1;
    }

   //create vector for fd structures and add the stucture of the serverFd (after listen() because the socket must be ready-to-use)
    std::vector<pollfd> fds; 
    pollfd serverStrctFd;
    createFdPollStrct(serverFd, serverStrctFd);    
    fds.push_back(serverStrctFd);   

    std::map<int, Client> clientsByFd;
    std::map<std::string, int> fdByNick;
    signal(SIGINT, handleSignal);

    int result = pollLoop(serverFd, fds, clientsByFd, fdByNick, pass);
    
    std::vector<pollfd>::iterator it;
    for(it = fds.begin(); it != fds.end(); ++it)
    {
        close(it->fd);
    }
    return result;
}


/*
SOURCES:
https://cplusplus.com
*/