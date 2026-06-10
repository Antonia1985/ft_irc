#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <iostream>

void print_prompt()
{
    std::cout << "CLIENT > " << std::flush;
}

int main()
{
    struct addrinfo hints{}, *res, *p;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", "8080", &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return 1;
    }

    int sockfd = -1;

    for (p = res; p != nullptr; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (!p)
    {
        std::cerr << "Connection failed\n";
        return 1;
    }

    pollfd fds[2];
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

    // ===== SERVER MESSAGE =====
    if (fds[1].revents & POLLIN)
    {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0)
        {
            std::cout << "\nDisconnected\n";
            break;
        }

        buffer[bytes] = '\0';

        // \33[2K clears the current line
        // \r returns the cursor to the start of the line
        std::cout << "\33[2K\rSERVER: " << buffer << std::flush;

        need_prompt = true;
    }

    // ===== USER INPUT =====
    if (fds[0].revents & POLLIN)
    {
        std::string line;
        std::getline(std::cin, line);

        line += "\r\n";
        send(sockfd, line.c_str(), line.size(), 0);

        need_prompt = true;
    }

    // ONLY PRINT PROMPT WHEN NEEDED
    if (need_prompt)
    {
        std::cout << "CLIENT > " << std::flush;
        need_prompt = false;
    }
}

    close(sockfd);
}
