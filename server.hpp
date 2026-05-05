#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <poll.h>
#include <vector>
#include <map>
#include "client.hpp"


void handleSignal(int signal);
int socketCreation();
int fdNonBlocking(int fd);
int reusableImmediately(int fd);
int bindSocket(int port,  int serverFd);
int listenSocket(int serverFd);
void createFdPollStrct(int serverFd, pollfd& serverStrctFd);
int pollLoop(int serverFd, std::vector<pollfd>& fds, std::map<int, Client>& clients);

#endif