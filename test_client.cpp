#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cctype>

// -------------------- utils --------------------

bool isInteger(const std::string &s)
{
    if (s.empty())
        return false;

    size_t i = 0;

    if (s[0] == '+' || s[0] == '-')
        i++;

    if (i == s.size())
        return false;

    for (; i < s.size(); i++)
    {
        if (!std::isdigit(s[i]))
            return false;
    }
    return true;
}

void print_prompt()
{
    std::cout << "CLIENT > " << std::flush;
}

// -------------------- main --------------------

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./client <port>\n";
        return 1;
    }

    std::string portStr = argv[1];

    if (!isInteger(portStr))
    {
        std::cerr << "Port must be a number\n";
        return 1;
    }

    int port = std::atoi(portStr.c_str());
    if (port < 1 || port > 65535)
    {
        std::cerr << "Port out of range\n";
        return 1;
    }

    // -------------------- connect --------------------

    struct addrinfo hints, *res, *p;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo("127.0.0.1", argv[1], &hints, &res);
    if (status != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        return 1;
    }

    int sockfd = -1;

    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(sockfd);
            sockfd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (sockfd < 0)
    {
        std::cerr << "Connection failed\n";
        return 1;
    }

    // -------------------- poll loop --------------------

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    char buffer[1024];
    bool need_prompt = true;

    print_prompt();

    while (true)
    {
        int ret = poll(fds, 2, -1);

        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // ---------------- server ----------------
        if (fds[1].revents & POLLIN)
        {
            ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0)
            {
                std::cout << "\nDisconnected\n";
                break;
            }

            buffer[bytes] = '\0';

            std::cout << "\33[2K\rSERVER > " << buffer << std::flush;

            need_prompt = true;
        }

        if (fds[0].revents & POLLIN)
        {
            std::string line;
            std::getline(std::cin, line);

            if (std::cin.eof())
                break;

            line += "\r\n";
            send(sockfd, line.c_str(), line.size(), 0);

            need_prompt = true;
        }

        // ---------------- prompt ----------------
        if (need_prompt)
        {
            print_prompt();
            need_prompt = false;
        }
    }

    close(sockfd);
    return 0;
}
