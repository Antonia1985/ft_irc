#include "parser.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

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

static void handlePing(int fd, std::string args)
{
    std::string msg;
    if(!args.empty())
        msg = "PONG" + args;
    else
        msg = "PONG";
    sendMsg(fd, msg);
}

static void handleNick(int fd, std::string args)
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

}

static void parseArgs(int fd, std::string command, std::string args)
{
    if (command == "PING")
        handlePing(fd, args);
    if (command == "NICK")
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
        handleMode(fd, args);
}



//find the COMMANDS and ARGUMENTS
void parse (int fd, std::string& line, std::map<int, Client>& clients)
{
    std::string command;
    std::string args;
    size_t pos;
    
    pos = line.find(' ', 0);
    if(pos != std::string::npos)
    {
        command = line.substr(0, pos);
        if(line.size() > pos+1)
        {
            line = line.substr(pos+1);
            pos = line.find(' ', 0);
            args = line.substr(pos+1, line.size());
        }
    }
    else
    {
        command = line;
        args = "";
    }
    parseArgs(fd, command, args);
}
