#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <string>
#include "parsedMessage.hpp"

void sendError(int clientFd, int error, ParsedMessage parsed, std::string nickname, std::string channel);

#endif