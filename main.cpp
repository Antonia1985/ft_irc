#include "server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>\n";
        return 1;
    }

    int port = std::atoi(argv[1]); //if port = abc then atoi("abc") = 0
    if (port <= 0 || port > 65535)
    {
        std::cerr << "The port number is not correct" << std::endl;
        return 1;
    }

    Server server(argv[1], argv[2]);
    server.start();
    return 0;
}


/*
SOURCES:
https://cplusplus.com
*/
