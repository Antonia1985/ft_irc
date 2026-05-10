#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include "client.hpp"
#include "parsedMessage.hpp"


ParsedMessage parseMessage(std::string line);
std::string toUpper(std::string s);

#endif