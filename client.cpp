#include "client.hpp"
#include <unistd.h>

Client::Client()
    : fd(-1),
      buffer(""),
      nickname(""),
      username(""),
      passOk(false),
      registered(false)
{
}

int Client::getFd() const { return fd; }
const std::string& Client::getBuffer() const { return buffer; }
const std::string& Client::getNickname() const { return nickname; }
const std::string& Client::getUsername() const { return username; }
bool Client::getPassOk() const { return passOk; }
bool Client::getRegistered() const { return registered; }

void Client::setFd(int f) { fd = f; }
void Client::setBuffer(const std::string& buf) { buffer = buf; }
void Client::setNickname(const std::string& nicknm) { nickname = nicknm; }
void Client::setUsername(const std::string& usernm) { username = usernm; }
void Client::setPassOk(bool p) { passOk = p; }
void Client::setRegistered(bool r) { registered = r; }

void Client::appendToBuffer(const std::string& data)
{
    buffer += data;
}

void Client::appendToBuffer(const char* data, size_t len)
{
    buffer.append(data, len);
}


void Client::eraseFromBuffer(size_t pos, size_t len)
{
    buffer.erase(pos, len);
}

void removeClient(std::vector<pollfd>::iterator it, 
                std::map<int, Client>& clients, std::vector<pollfd>& fds)
{
    int fd = it->fd;
    close(fd);
    clients.erase(fd);
    fds.erase(it);
}

/*
Iterator invalidation risk
removeClient(it, clients, fds); erases from fds while iterating over it . 
Your break avoids some damage, but it is fragile. 
Better make removeClient return the next iterator later.
*/

