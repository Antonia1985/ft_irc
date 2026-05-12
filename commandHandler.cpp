#include "commandHandler.hpp"
#include "Channel.hpp"
#include "parser.hpp"
#include "errors.hpp"
#include <iostream>
#include <sys/socket.h>
#include <string>
#include <cctype>


void handleJoin(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels) {
    if (parsed.params.empty())
    {
        sendError(fd, 461, parsed, clientsByFd[fd].getNickname(), "");
        return;
    }
    
    std::string channelName = parsed.params[0];
    std::map<std::string, Channel>::iterator it = channels.find(channelName);

    if(it != channels.end())
    {
        it->second.addUser(fd);
        clientsByFd[fd].addChannel(channelName);
        std::cout << "exist" << std::endl;
    }
    else
    {
        channels[channelName] = Channel(channelName);
        channels[channelName].addUser(fd);
        channels[channelName].addOperator(fd);
        clientsByFd[fd].addChannel(channelName);
        std::cout << "doesnt exist" << std::endl;
    }
}

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

static int handlePass(int fd, const ParsedMessage& parsed, 
                        std::map<int, Client>& clientsByFd, const std::string& serverPass) //returns 0 when client must disconnect
{
    Client& client = clientsByFd[fd];
    std::string nickname = client.getNickname();   

    if(client.getRegistered() == true)  //if already registered
    {
        sendError(fd, 462, parsed, nickname, "");
        return 1;
    }
    if(parsed.params.size() == 0) //if no arguments
    {
        sendError(fd, 461, parsed, nickname, "");
        return 1;
    }
    if(serverPass != parsed.params[0]) //if password doesn't match server's password (if extra args exist, they are ignored like real irc)
    {
        sendError(fd, 464, parsed, nickname, "");
        std::cout << clientsByFd[fd].getPassOk() << std::endl; //testin the clientsByFd is filled
        return 0;
    }
    //success case
    client.setPassOk(true);
    return 1;
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
    if (nick.size() >9)
        return 3;

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

static int validUser(int fd, const ParsedMessage& parsed)
{
    (void)fd;
    std::string user = parsed.params[0];
    for(size_t i = 0; i < user.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(user[i]);
        if(std::isalnum(static_cast<unsigned char>(c))
            || c == '_' || c == '-') 
        {
            
            return 0;
        }
    }
    return 1;
}

std::string buildRealName(const ParsedMessage& parsed)
{
    std::string realName = parsed.params[3];
    for(size_t i = 4; i < parsed.params.size(); i++)
    {
        realName += std::string(" ") + parsed.params[i];
    }
    return realName;
}
static void handleUser(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd)
{
    Client& client = clientsByFd[fd];
    std::string nickname = client.getNickname();   

    // handling <username>
    if(parsed.params.size() < 4) //if less arguments
    {
        sendError(fd, 461, parsed, nickname, "");
        return;
    }
    if(client.getRegistered() == true)  //if already registered: do not let USER change after registration
    {
        sendError(fd, 462, parsed, nickname, "");
        return;
    }
    if(!validUser(fd, parsed)) //if username has an invalid char
    {
        sendMsg(fd, "ERROR :Invalid username (must contain only letters/numbers)");
    }
    if(parsed.params[0].size() > 10) //if username is big
    {
        sendMsg(fd, "ERROR :Invalid username (must contain maximum 10 digits)");
        return;
    }
    //success case
    clientsByFd[fd].setUsername(parsed.params[0]);

    // handling <realname>
    std::string realName;
    if (parsed.params.size() > 3) // if more than 4 params then built the real name appending the last params
    {
        realName = buildRealName(parsed);
    }
    if (realName.size() > 50)//if real name is big
    {
        sendMsg(fd, "ERROR :Invalid username (must contain maximum 10 digits)");
        return;
    }
}
/*static void handleJoin(int fd, std::string args)
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


int handleCommand(int fd, ParsedMessage parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, const std::string& serverPass, std::map<std::string, Channel>& channels)
{
    if (parsed.command == "PING")
    {
        handlePing(fd, parsed);
    }        
    else if (parsed.command == "NICK")
    {
        handleNick(fd, parsed, clientsByFd, fdByNickUp);
    }        
    else if (parsed.command == "PASS")
    {
        if(!handlePass(fd, parsed, clientsByFd, serverPass))
            return 0;
    }
    else if (parsed.command == "USER")
        handleUser(fd, parsed,clientsByFd);
    /*else if (command == "JOIN")
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
    else if(parsed.command == "JOIN")
        handleJoin(fd, parsed, clientsByFd, channels);
    else
    {
        sendError(fd, 421, parsed, "", "");
    }
    return 1;
}


/*
----------------------------------------------
Complete realistic flow

Client → Server:
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :Hello!

Server → Client:
001 alice :Welcome to the IRC server

-----------------------------------------------
PASS:

once correct PASS received → passOk = true
later wrong PASS does NOT unset it
Because PASS is usually treated as:
authentication succeeded
not as a continuously mutable state.

----------------------------------------------
NICK:

must not be empty
must not contain spaces
should start with a letter
should not contain # , :

----------------------------------------------
USER:
USER <username> 0 * :<realname>

<username>
- required during registration
- not necessarily unique
- allowed chars:
    a-z
    A-Z
    0-9
    _
    -
- reasonable max: 9 or 10 chars, or define your own limit
- case-sensitive is okay, because it is not the main identity
- do not let USER change after registration

<realname>
-realname is REQUIRED. 
 Although some irc servers allow it to be " " space. My implementation requires it.
-you should reject:
    NUL   '\0'
    CR    '\r'
    LF    '\n'
    but they are already handled when the server stores the line into buffer
-permissive logic:
 Everything from param[3] and after are part of real name
-size: 50 chars are ok (IRC: 512  bytes for the entire irc line)
*/
