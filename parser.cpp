#include "parser.hpp"
#include "parsedMessage.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <cctype>
#include <string>

static size_t posOfWhitespace(std::string line)
{
    size_t pos;

    if (!line.empty() && (line[0] == ' ' || line[0] == '\t'))

    pos = line.find(' ', 0);
}

static std::string trimLeadingWhitespace(std::string line)
{
    while (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
        line.erase(0, 1);
    
    return line;
}

static std::string toUpper(std::string s)
{
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    }
    return s;
}

void sendMsg(int clientFd, std::string msg) // TO DO!!!
{
    //std::string msg = "Server received your message\n";
    if (send(clientFd, msg.c_str(), msg.size(), 0) <= 0)
    {
        std::cerr << "send() failed!" << std::endl;
        //close(clientFd);
        //close(serverFd);
        //return 1;
    }
    /*else
    {
        std::cout << "message sent to client!" << std::endl;
    }*/
}

static void handlePing(int fd, ParsedMessage parsed)
{
    std::string msg;
    if(!parsed.params.empty())
        msg = "PONG" + ' ' + parsed.params[0] + "\r\n";
    else
        msg = "PONG\r\n";
    sendMsg(fd, msg);
}

/*static void handleNick(int fd, std::string args)
{

}

static void handlePass(int fd, std::string args)
{

}

static void handleUser(int fd, std::string args)
{

}

static void handleJoin(int fd, std::string args)
{

}

static void handlePart(int fd, std::string args)
{

}

static void handlePrivmsg(int fd, std::string args)
{

}

static void handleNotice(int fd, std::string args)
{

}

static void handleKick(int fd, std::string args)
{

}

static void handleInvite(int fd, std::string args)
{

}

static void handleTopic(int fd, std::string args)
{

}

static void handleMode(int fd, std::string args)
{

}*/

static void handleCommand(int fd, ParsedMessage parsed)
{
    if (parsed.command == "PING")
        handlePing(fd, parsed);
    /*if (command == "NICK")
        handleNick(fd, args);
    if (command == "PASS")
        handlePass(fd, args);
    if (command == "USER")
        handleUser(fd, args);   
    if (command == "JOIN")
        handleJoin(fd, args);
    if (command == "PART")
        handlePart(fd, args);
    if (command == "PRIVMSG")
        handlePrivmsg(fd, args);
    if (command == "NOTICE")
        handleNotice(fd, args);
    if (command == "KICK")
        handleKick(fd, args);
    if (command == "INVITE")
        handleInvite(fd, args);
    if (command == "TOPIC")
        handleTopic(fd, args);
    if (command == "MODE")
        handleMode(fd, args);*/
}

static ParsedMessage parseMessage(std::string line)
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
void parse (int fd, std::string& line, std::map<int, Client>& clients)
{
    ParsedMessage parsed = parseMessage(line);
    handleCommand(fd, parsed);
}


/*
These should all parse identically:

NICK Jenny
   NICK Jenny
\t\tNICK Jenny
 \t  NICK Jenny
NICK\tJenny
NICK \t  Jenny
*/