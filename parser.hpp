#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <map>
#include "client.hpp"

void parse (int fd, std::string& line, std::map<int, Client>& clients);

#endif