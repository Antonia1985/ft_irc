#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "parsedMessage.hpp"
#include "client.hpp"
#include <map>

int handleCommand(int fd, ParsedMessage parsed, std::map<int, Client>& clientsByFd, 
                    std::map<std::string, int>& fdByNickUp, const std::string& serverPass);
void sendMsg(int clientFd, std::string msg);

#endif