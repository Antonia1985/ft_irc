#ifndef PARSEDMESSAGE_HPP
#define PARSEDMESSAGE_HPP

#include <string>
#include <vector>

struct ParsedMessage
{
    std::string command;
    std::vector<std::string> params;
    bool lastParamTrailing;

    ParsedMessage();
};

#endif