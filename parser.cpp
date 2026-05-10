#include "parser.hpp"
//#include "errors.hpp"
//#include <iostream>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <cctype>


static std::string trimLeadingWhitespace(std::string line)
{
    while (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
        line.erase(0, 1);
    
    return line;
}

std::string toUpper(std::string s)
{
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    }
    return s;
}


ParsedMessage parseMessage(std::string line)
{
    ParsedMessage parsed;
    std::string command;
    std::string args;
    size_t posSpace;

    line = trimLeadingWhitespace(line); //trim leading white space and tabs
    
    posSpace = line.find_first_of(" \t");
    if(posSpace != std::string::npos) //if line has a space
    {
        command = line.substr(0, posSpace); //then extract the command        
        if(line.size() > posSpace+1) //if line has more characters after space
        {
            args = line.substr(posSpace+1);// then extract the whole args
        }
    }
    else //if line has no space
    {
        command = line; //the command is the full line
        args = "";  //and args are empty
    }    
    parsed.command = toUpper(command); // save the command in upper case

    std::string arg;
    while(!args.empty())
    {
        args = trimLeadingWhitespace(args); // trim leading ws for the args string
        if (args.empty())
        {
            break;
        }

        if(args[0] == ':') //ch
        {
            parsed.params.push_back(args.substr(1));
            parsed.lastParamTrailing = true;
            break;
        }
        
        posSpace = args.find_first_of(" \t", 0); //check if an other space exists later
        if (posSpace != std::string::npos) //if yes 
        {
            arg = args.substr(0, posSpace); // extract the first argument
            parsed.params.push_back(arg); //put it in the ParsedMessage struct
            args = args.substr(posSpace+1); // and delete it from the args string
        }
        else //if no other white space exists
        {
            arg = args.substr(0);
            parsed.params.push_back(arg);
            args="";
        }            
    }
    
    return parsed;
}

//find the COMMANDS and ARGUMENTS
/*void parse (int fd, std::string& line, std::map<int, Client>& clients)
{
    (void)clients;
    ParsedMessage parsed = parseMessage(line);
    handleCommand(fd, parsed);
}*/


/*
These should all parse identically:

NICK Jenny
   NICK Jenny
\t\tNICK Jenny
 \t  NICK Jenny
NICK\tJenny
NICK \t  Jenny
*/