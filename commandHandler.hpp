#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "parsedMessage.hpp"
#include "client.hpp"
#include "Channel.hpp"
#include <map>

int handleCommand(int fd, ParsedMessage parsed, std::map<int, Client>& clientsByFd, 
                    std::map<std::string, int>& fdByNickUp, const std::string& serverPass, std::map<std::string, Channel>& channels);
void sendMsg(int clientFd, std::string msg);
void handleJoin(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels);
void handlePart(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels);
int handleQuit(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels);
void handleKick(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels);
void handleInvite(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels);
void handleTopic(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels);
void handleMode(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels);

#endif
