#include "commandHandler.hpp"
#include "parser.hpp"
#include "errors.hpp"
#include <iostream>
#include <sys/socket.h>
#include <string>


void sendMsg(int clientFd, std::string msg)
{
    std::string fullMsg = msg + "\r\n";
    if (send(clientFd, fullMsg.c_str(), fullMsg.size(), 0) <= 0)
    {
        std::cerr << "send() failed!" << std::endl;
    }
}

static void handlePing(int fd, const ParsedMessage& parsed)
{
    std::string msg;
    if(!parsed.params.empty())
    {        
        if((parsed.lastParamTrailing == true) && (parsed.params.size() == 1))
            msg = std::string("PONG :") + parsed.params[0];
        else
            msg = std::string("PONG ") + parsed.params[0];
    }        
    else
        msg = "PONG";
    sendMsg(fd, msg);
}

int validNick(const std::string& nick)
{
    if (nick.empty())
        return 2;

    if (!std::isalpha(nick[0]))
        return 3;

    for (size_t i = 0; i < nick.size(); i++)
    {
        if (nick[i] == ' ' || nick[i] == '#' ||
            nick[i] == ':' || nick[i] == ',')
            return 3;
    }

    return 1;
}

static void handleNick(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp)
{
    std::string oldnickn = clientsByFd[fd].getNickname();//I take the nickname from clients, it's either existing or empty
    if (parsed.params.empty()) //if empty must be the first to check eitherwise: toUpper(parsed.params[0]) will crash
    {
        sendError(fd, 431, parsed, oldnickn, "");
        return;
    }
    std::string nickToUp = toUpper(parsed.params[0]);

    //check if nickname after command is erroneus
    if(validNick(parsed.params[0]) == 3) 
    {
        sendError(fd, 432, parsed, oldnickn, "");
        return;
    }

    //check if nickname already exists    
    if(fdByNickUp.find(nickToUp) != fdByNickUp.end()) 
    {
        sendError(fd, 433, parsed, oldnickn, "");
        return;
    }

    //success case:
    clientsByFd[fd].setNickname(parsed.params[0]);
    clientsByFd[fd].setNicknameToUp(nickToUp);

    //search if oldnickname exists in fdByNickUp, if yes erase it
    std::map<std::string, int>::iterator it;
    it = fdByNickUp.find(toUpper(oldnickn));
    if (it != fdByNickUp.end())
    {
        fdByNickUp.erase(it);
    }    
    fdByNickUp[nickToUp] = fd; // and add in the map fdByNickUp the new nickname
    //std::cout << clientsByFd[fd].getNicknameToUp() << std::endl; //testin the clientsByFd is filled 
}

/*static void handlePass(int fd, std::string args)
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

void handleCommand(int fd, ParsedMessage parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp)
{
    if (parsed.command == "PING")
        handlePing(fd, parsed);
    else if (parsed.command == "NICK")
        handleNick(fd, parsed, clientsByFd, fdByNickUp);
    /*else if (command == "PASS")
        handlePass(fd, args);
    else if (command == "USER")
        handleUser(fd, args);   
    else if (command == "JOIN")
        handleJoin(fd, args);
    else if (command == "PART")
        handlePart(fd, args);
    else if (command == "PRIVMSG")
        handlePrivmsg(fd, args);
    else if (command == "NOTICE")
        handleNotice(fd, args);
    else if (command == "KICK")
        handleKick(fd, args);
    else if (command == "INVITE")
        handleInvite(fd, args);
    else if (command == "TOPIC")
        handleTopic(fd, args);
    else if (command == "MODE")
        handleMode(fd, args);*/
    //validate the command
    else
    {
        sendError(fd, 421, parsed, "", "");
    }
}


/*

A nickname:

must not be empty
must not contain spaces
should start with a letter
should not contain # , :

*/