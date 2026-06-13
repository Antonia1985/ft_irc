#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "client.hpp"
#include "Channel.hpp"

struct ClientBuffer
{
    std::string incoming;
    std::string outgoing;
};

class Server
{
public:
    Server(const std::string& port, std::string password);
    ~Server();
    bool start();
    void stop();

    // Used by sendMsg (commandHandler.cpp) to queue outgoing data
    void prepare_send(int fd, const std::string& msg);

private:
    std::string                 port;
    std::string                 password;
    int                         sockfd;
    int                         max_conn;
    std::vector<pollfd>         fds;
    std::map<int, ClientBuffer> client_buffers;
    std::map<int, Client>       clientsByFd;
    std::map<std::string, int>  fdByNickUp;
    std::map<std::string, Channel> channels;
    static bool                 running;

private:
    bool setup_socket();
    void setup_poll();
    void run();
    void accept_client();
    bool handle_client(size_t i);
    void flush_client(size_t i);
    void disconnect_client(size_t i);
    void process_message(int client_fd, const std::string& msg);
    static void signal_handler(int sig);
};

#endif
