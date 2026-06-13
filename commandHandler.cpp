#include "commandHandler.hpp"
#include "Channel.hpp"
#include "parser.hpp"
#include "errors.hpp"
#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <string>
#include <cctype>
#include <sstream>
#include <vector>
#include <cstdlib>

/*void debugPrintClients(const std::map<int, Client>& clientsByFd)
{
    std::map<int, Client>::const_iterator it;

    std::cout << "\n========== CLIENTS DEBUG ==========" << std::endl;

    if (clientsByFd.empty())
    {
        std::cout << "No clients connected." << std::endl;
        std::cout << "===================================\n" << std::endl;
        return;
    }

    for (it = clientsByFd.begin(); it != clientsByFd.end(); ++it)
    {
        const int fd = it->first;
        const Client& client = it->second;

        std::cout << "fd: " << fd << std::endl;
        std::cout << "nickname: [" << client.getNickname() << "]" << std::endl;
        std::cout << "username: [" << client.getUsername() << "]" << std::endl;
        std::cout << "realname: [" << client.getRealname() << "]" << std::endl;
        std::cout << "passOk: " << client.getPassOk() << std::endl;
        std::cout << "registered: " << client.getRegistered() << std::endl;
        std::cout << "-----------------------------------" << std::endl;
    }

    std::cout << "===================================\n" << std::endl;
}*/



// Defined in server.cpp; gives access to Server::prepare_send() without
// passing the Server instance through every command handler call.
extern Server* g_server;

void sendMsg(int clientFd, std::string msg)
{
    std::string fullMsg = msg + "\r\n";
    if (g_server)
        g_server->prepare_send(clientFd, fullMsg);
}

void sendToChannel(int senderFd, std::string msg, Channel& channel)
{
    const std::set<int>& channelUsers = channel.getUsers();
    std::set<int>::const_iterator it;
    for(it = channelUsers.begin(); it != channelUsers.end(); ++it)
    {
        if (*it == senderFd)
            continue;
        sendMsg(*it, msg);
    }
}

static std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

static void broadcastToChannel(const std::string& msg, Channel& channel)
{
    const std::set<int>& users = channel.getUsers();
    for (std::set<int>::const_iterator it = users.begin(); it != users.end(); ++it)
    {
        sendMsg(*it, msg);
    }
}



//PING
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

//PASS
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
        //std::cout << clientsByFd[fd].getPassOk() << std::endl; //testin the clientsByFd is filled
        return 0;
    }
    //success case
    client.setPassOk(true);
    //set as registered if..
    if(!client.getNickname().empty() && !client.getUsername().empty()) //if all 3 required fileds for registration are ok
    {
        client.setRegistered(true);
        sendNotification(fd, 1 , parsed, nickname, "");
    }

    return 1;
}

//NICK
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
    Client& client = clientsByFd[fd];
    std::string oldnickn = client.getNickname();
    
    //if empty must be the first to check eitherwise: toUpper(parsed.params[0]) will crash
    if (parsed.params.empty()) 
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

    // if already registerd but changed the nickname
    if(clientsByFd[fd].getRegistered())
    {
        std::string msg = ":" + oldnickn + "!" + clientsByFd[fd].getUsername() + "@localhost NICK :" + parsed.params[0];
        sendMsg(fd, msg);
    }
    //set as registered if..
    if(clientsByFd[fd].getPassOk() && ! clientsByFd[fd].getUsername().empty() && !clientsByFd[fd].getRegistered()) //if all 3 required fileds for registration are ok
    {
        clientsByFd[fd].setRegistered(true);
        sendNotification(fd, 1 , parsed, parsed.params[0], "");
    }
}

//USER
static int validUser(int fd, const ParsedMessage& parsed)
{
    (void)fd;
    std::string user = parsed.params[0];
    for(size_t i = 0; i < user.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(user[i]);
        if(!std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') 
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
    clientsByFd[fd].setRealname(realName);

    //set as registered if..
    if(clientsByFd[fd].getPassOk() && !clientsByFd[fd].getNickname().empty()) //if all 3 required fileds for registration are ok
    {
        clientsByFd[fd].setRegistered(true);
        sendNotification(fd, 1 , parsed, parsed.params[0], "");
    }
}

//PRIVMSG
static void handlePrivmsg(int senderFd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, 
                            std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels)
{
    std::string host = "localhost";
    std::string senderNick = clientsByFd[senderFd].getNickname();
    std::string receiver = "";

    if(parsed.params.size() == 0) //if no arguments at all
    {
        sendError(senderFd, 461, parsed, senderNick, "");
        return;
    }

    if(parsed.params.size() == 1) //if no message exists
    {
        sendError(senderFd, 412, parsed, senderNick, "");
        return;
    }

    receiver = parsed.params[0];
    if(receiver[0] == '#') //if it looks like a channel
    {
        if (channels.find(receiver) == channels.end()) // if the channel does not exist
        {
            sendError(senderFd, 403, parsed, senderNick, receiver);
            return;
        }

        //if channel exists does the sender has access???
        Channel& chan = channels[receiver];
        if(chan.hasUser(senderFd)) //if the sender has access to channel
        {
            //success case
            std::string msg = std::string(":") + senderNick + std::string("!") 
                            + clientsByFd[senderFd].getUsername() + "@" +  host 
                            + " PRIVMSG "+ receiver + std::string(" :") + parsed.params[1];
            sendToChannel(senderFd, msg, chan);
        }
        else
        {
            sendError(senderFd, 442, parsed, senderNick, receiver);
            return;
        }
        
    }
    else //it looks like a nickname
    {
        if(fdByNickUp.find(toUpper(receiver)) == fdByNickUp.end()) // if its not a nickname
        {
            sendError(senderFd, 401, parsed, senderNick, "");
            return;
        }
        //success case
        std::string nickUp = toUpper(receiver);
        int receiverFd = fdByNickUp[nickUp];
        std::string msg = std::string(":") + senderNick + std::string("!") 
                        + clientsByFd[senderFd].getUsername() + "@" +  host 
                        + " PRIVMSG "+ receiver + std::string(" :") + parsed.params[1];
        sendMsg(receiverFd, msg);
    }    
}



void handleJoin(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.empty())
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::vector<std::string> channelNames = split(parsed.params[0], ',');
    std::vector<std::string> keys;
    if (parsed.params.size() > 1)
    {
        keys = split(parsed.params[1], ',');
    }

    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string channelName = channelNames[i];
        if (channelName.empty() || channelName[0] != '#' || channelName.size() == 1)
        {
            sendError(fd, 403, parsed, nick, channelName);
            continue;
        }

        std::string providedKey = (i < keys.size()) ? keys[i] : "";

        std::map<std::string, Channel>::iterator it = channels.find(channelName);
        if (it != channels.end())
        {
            Channel& channel = it->second;

            // invite-only check
            if (channel.hasMode('i') && !channel.isInvited(client.getNicknameToUp()))
            {
                sendError(fd, 473, parsed, nick, channelName);
                continue;
            }

            // key check
            if (channel.hasMode('k') && channel.getKey() != providedKey)
            {
                sendError(fd, 475, parsed, nick, channelName);
                continue;
            }

            // limit check
            if (channel.hasMode('l') && channel.getUserLimit() > 0)
            {
                if (static_cast<int>(channel.getUsers().size()) >= channel.getUserLimit())
                {
                    sendError(fd, 471, parsed, nick, channelName);
                    continue;
                }
            }

            if (channel.hasUser(fd))
            {
                continue;
            }

            channel.addUser(fd);
            client.addChannel(channelName);
            channel.uninviteUser(client.getNicknameToUp());
        }
        else
        {
            channels[channelName] = Channel(channelName);
            Channel& channel = channels[channelName];
            channel.addUser(fd);
            channel.addOperator(fd);
            client.addChannel(channelName);
        }

        Channel& channel = channels[channelName];
        std::string joinBroadcastMsg = ":" + nick + "!" + user + "@localhost JOIN " + channelName;
        broadcastToChannel(joinBroadcastMsg, channel);

        if (channel.getTopic().empty())
        {
            sendNotification(fd, 331, parsed, nick, channelName);
        }
        else
        {
            ParsedMessage topicNotice;
            topicNotice.params.push_back(channel.getTopic());
            sendNotification(fd, 332, topicNotice, nick, channelName);
        }

        std::string nameList = "";
        const std::set<int>& users = channel.getUsers();
        for (std::set<int>::const_iterator uIt = users.begin(); uIt != users.end(); ++uIt)
        {
            if (!nameList.empty())
                nameList += " ";
            if (channel.isOperator(*uIt))
                nameList += "@";
            nameList += clientsByFd[*uIt].getNickname();
        }

        ParsedMessage pNotice;
        pNotice.params.push_back(nameList);
        sendNotification(fd, 353, pNotice, nick, channelName);
        sendNotification(fd, 366, pNotice, nick, channelName);
    }
}

void handleCap(int fd, const ParsedMessage& parsed)
{
    if (parsed.params.size() > 0 && parsed.params[0] == "LS")
    {
        sendMsg(fd, ":ircserv CAP * LS :");
    }
    else if (parsed.params.size() > 0 && parsed.params[0] == "REQ")
    {
        std::string reqCap = (parsed.params.size() > 1) ? parsed.params[1] : "";
        sendMsg(fd, ":ircserv CAP * NAK :" + reqCap);
    }
}

void handlePart(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.empty())
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::vector<std::string> channelNames = split(parsed.params[0], ',');
    std::string reason = (parsed.params.size() > 1) ? parsed.params[1] : "";

    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string channelName = channelNames[i];
        std::map<std::string, Channel>::iterator it = channels.find(channelName);
        if (it == channels.end())
        {
            sendError(fd, 403, parsed, nick, channelName);
            continue;
        }

        Channel& channel = it->second;
        if (!channel.hasUser(fd))
        {
            sendError(fd, 442, parsed, nick, channelName);
            continue;
        }

        std::string partMsg = ":" + nick + "!" + user + "@localhost PART " + channelName;
        if (!reason.empty())
            partMsg += " :" + reason;
        
        broadcastToChannel(partMsg, channel);
        
        channel.removeUser(fd);
        channel.removeOperator(fd);
        client.removeChannel(channelName);

        if (channel.getUsers().empty())
        {
            channels.erase(it);
        }
    }
}

int handleQuit(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    std::string reason = (parsed.params.size() > 0) ? parsed.params[0] : "leaving";
    std::string quitMsg = ":" + nick + "!" + user + "@localhost QUIT :" + reason;

    const std::set<std::string>& joinedChans = client.getChannels();
    std::vector<std::string> chansToLeave(joinedChans.begin(), joinedChans.end());

    for (size_t i = 0; i < chansToLeave.size(); ++i)
    {
        std::map<std::string, Channel>::iterator it = channels.find(chansToLeave[i]);
        if (it != channels.end())
        {
            Channel& channel = it->second;
            const std::set<int>& users = channel.getUsers();
            for (std::set<int>::const_iterator uIt = users.begin(); uIt != users.end(); ++uIt)
            {
                if (*uIt != fd)
                {
                    sendMsg(*uIt, quitMsg);
                }
            }
            channel.removeUser(fd);
            channel.removeOperator(fd);
            
            if (channel.getUsers().empty())
            {
                channels.erase(it);
            }
        }
    }
    return 0;
}

void handleKick(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.size() < 2)
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::string channelName = parsed.params[0];
    std::string targetNick = parsed.params[1];
    std::string reason = (parsed.params.size() > 2) ? parsed.params[2] : "";

    std::map<std::string, Channel>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        sendError(fd, 403, parsed, nick, channelName);
        return;
    }

    Channel& channel = it->second;
    if (!channel.hasUser(fd))
    {
        sendError(fd, 442, parsed, nick, channelName);
        return;
    }

    if (!channel.isOperator(fd))
    {
        sendError(fd, 482, parsed, nick, channelName);
        return;
    }

    std::string targetNickUp = toUpper(targetNick);
    std::map<std::string, int>::iterator tIt = fdByNickUp.find(targetNickUp);
    if (tIt == fdByNickUp.end())
    {
        sendError(fd, 401, parsed, nick, "");
        return;
    }

    int targetFd = tIt->second;
    if (!channel.hasUser(targetFd))
    {
        ParsedMessage pErr = parsed;
        sendError(fd, 441, pErr, nick, channelName);
        return;
    }

    std::string kickMsg = ":" + nick + "!" + user + "@localhost KICK " + channelName + " " + targetNick;
    if (!reason.empty())
        kickMsg += " :" + reason;
    else
        kickMsg += " :Kicked by operator";

    broadcastToChannel(kickMsg, channel);

    channel.removeUser(targetFd);
    channel.removeOperator(targetFd);
    clientsByFd[targetFd].removeChannel(channelName);

    if (channel.getUsers().empty())
    {
        channels.erase(it);
    }
}

void handleInvite(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.size() < 2)
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::string targetNick = parsed.params[0];
    std::string channelName = parsed.params[1];

    std::map<std::string, Channel>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        sendError(fd, 403, parsed, nick, channelName);
        return;
    }

    Channel& channel = it->second;
    if (!channel.hasUser(fd))
    {
        sendError(fd, 442, parsed, nick, channelName);
        return;
    }

    if (channel.hasMode('i') && !channel.isOperator(fd))
    {
        sendError(fd, 482, parsed, nick, channelName);
        return;
    }

    std::string targetNickUp = toUpper(targetNick);
    std::map<std::string, int>::iterator tIt = fdByNickUp.find(targetNickUp);
    if (tIt == fdByNickUp.end())
    {
        sendError(fd, 401, parsed, nick, "");
        return;
    }

    int targetFd = tIt->second;
    if (channel.hasUser(targetFd))
    {
        sendError(fd, 443, parsed, nick, channelName);
        return;
    }

    channel.inviteUser(targetNickUp);

    std::string inviteMsg = ":" + nick + "!" + user + "@localhost INVITE " + targetNick + " " + channelName;
    sendMsg(targetFd, inviteMsg);

    ParsedMessage pNotice;
    pNotice.params.push_back(targetNick);
    sendNotification(fd, 341, pNotice, nick, channelName);
}

void handleTopic(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.empty())
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::string channelName = parsed.params[0];
    std::map<std::string, Channel>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        sendError(fd, 403, parsed, nick, channelName);
        return;
    }

    Channel& channel = it->second;
    if (!channel.hasUser(fd))
    {
        sendError(fd, 442, parsed, nick, channelName);
        return;
    }

    if (parsed.params.size() < 2)
    {
        if (channel.getTopic().empty())
        {
            sendNotification(fd, 331, parsed, nick, channelName);
        }
        else
        {
            ParsedMessage topicNotice;
            topicNotice.params.push_back(channel.getTopic());
            sendNotification(fd, 332, topicNotice, nick, channelName);
        }
    }
    else
    {
        if (channel.hasMode('t') && !channel.isOperator(fd))
        {
            sendError(fd, 482, parsed, nick, channelName);
            return;
        }

        std::string newTopic = parsed.params[1];
        channel.setTopic(newTopic);

        std::string topicBroadcastMsg = ":" + nick + "!" + user + "@localhost TOPIC " + channelName + " :" + newTopic;
        broadcastToChannel(topicBroadcastMsg, channel);
    }
}

void handleMode(int fd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];
    std::string nick = client.getNickname();
    std::string user = client.getUsername();
    if (user.empty()) user = "unknown";

    if (parsed.params.empty())
    {
        sendError(fd, 461, parsed, nick, "");
        return;
    }

    std::string channelName = parsed.params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        if (toUpper(channelName) == client.getNicknameToUp())
        {
            sendMsg(fd, ":" + nick + " MODE " + nick + " :+");
            return;
        }
        else
        {
            sendMsg(fd, ":ircserv 502 " + nick + " :Cant change mode for other users");
            return;
        }
    }

    std::map<std::string, Channel>::iterator it = channels.find(channelName);
    if (it == channels.end())
    {
        sendError(fd, 403, parsed, nick, channelName);
        return;
    }

    Channel& channel = it->second;
    if (!channel.hasUser(fd))
    {
        sendError(fd, 442, parsed, nick, channelName);
        return;
    }

    if (parsed.params.size() < 2)
    {
        ParsedMessage pNotice;
        pNotice.params.push_back(channel.getModesString());
        std::string paramsStr = channel.getModesParamsString();
        if (!paramsStr.empty())
            pNotice.params.push_back(paramsStr);
        sendNotification(fd, 324, pNotice, nick, channelName);
        return;
    }

    if (!channel.isOperator(fd))
    {
        sendError(fd, 482, parsed, nick, channelName);
        return;
    }

    std::string modeString = parsed.params[1];
    std::string appliedModes = "";
    std::string appliedParams = "";
    char sign = '+';
    size_t paramIdx = 2;

    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char c = modeString[i];
        if (c == '+' || c == '-')
        {
            sign = c;
            continue;
        }

        if (c == 'i')
        {
            if (sign == '+')
            {
                if (!channel.hasMode('i'))
                {
                    channel.addMode('i');
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                    {
                        if (appliedModes.empty() || (appliedModes.find('+') == std::string::npos && sign == '+'))
                            appliedModes += "+";
                        else if (appliedModes[appliedModes.size() - 1] == '-')
                            appliedModes += "+";
                    }
                    appliedModes += "i";
                }
            }
            else
            {
                if (channel.hasMode('i'))
                {
                    channel.removeMode('i');
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                    {
                        appliedModes += "-";
                    }
                    appliedModes += "i";
                }
            }
        }
        else if (c == 't')
        {
            if (sign == '+')
            {
                if (!channel.hasMode('t'))
                {
                    channel.addMode('t');
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                    {
                        if (appliedModes.empty() || (appliedModes.find('+') == std::string::npos && sign == '+'))
                            appliedModes += "+";
                        else if (appliedModes[appliedModes.size() - 1] == '-')
                            appliedModes += "+";
                    }
                    appliedModes += "t";
                }
            }
            else
            {
                if (channel.hasMode('t'))
                {
                    channel.removeMode('t');
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                    {
                        appliedModes += "-";
                    }
                    appliedModes += "t";
                }
            }
        }
        else if (c == 'k')
        {
            if (sign == '+')
            {
                if (paramIdx < parsed.params.size())
                {
                    std::string keyVal = parsed.params[paramIdx++];
                    channel.addMode('k');
                    channel.setKey(keyVal);
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                    {
                        if (appliedModes.empty() || (appliedModes.find('+') == std::string::npos && sign == '+'))
                            appliedModes += "+";
                        else if (appliedModes[appliedModes.size() - 1] == '-')
                            appliedModes += "+";
                    }
                    appliedModes += "k";
                    if (!appliedParams.empty()) appliedParams += " ";
                    appliedParams += keyVal;
                }
            }
            else
            {
                if (channel.hasMode('k'))
                {
                    if (paramIdx < parsed.params.size())
                        paramIdx++;
                    channel.removeMode('k');
                    channel.setKey("");
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                    {
                        appliedModes += "-";
                    }
                    appliedModes += "k";
                }
            }
        }
        else if (c == 'l')
        {
            if (sign == '+')
            {
                if (paramIdx < parsed.params.size())
                {
                    std::string limitVal = parsed.params[paramIdx++];
                    int limit = std::atoi(limitVal.c_str());
                    if (limit > 0)
                    {
                        channel.addMode('l');
                        channel.setUserLimit(limit);
                        if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                        {
                            if (appliedModes.empty() || (appliedModes.find('+') == std::string::npos && sign == '+'))
                                appliedModes += "+";
                            else if (appliedModes[appliedModes.size() - 1] == '-')
                                appliedModes += "+";
                        }
                        appliedModes += "l";
                        if (!appliedParams.empty()) appliedParams += " ";
                        appliedParams += limitVal;
                    }
                }
            }
            else
            {
                if (channel.hasMode('l'))
                {
                    channel.removeMode('l');
                    channel.setUserLimit(-1);
                    if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                    {
                        appliedModes += "-";
                    }
                    appliedModes += "l";
                }
            }
        }
        else if (c == 'o')
        {
            if (paramIdx < parsed.params.size())
            {
                std::string targetNick = parsed.params[paramIdx++];
                std::string targetNickUp = toUpper(targetNick);
                std::map<std::string, int>::iterator tIt = fdByNickUp.find(targetNickUp);
                if (tIt != fdByNickUp.end())
                {
                    int targetFd = tIt->second;
                    if (channel.hasUser(targetFd))
                    {
                        if (sign == '+')
                        {
                            channel.addOperator(targetFd);
                            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                            {
                                if (appliedModes.empty() || (appliedModes.find('+') == std::string::npos && sign == '+'))
                                    appliedModes += "+";
                                else if (appliedModes[appliedModes.size() - 1] == '-')
                                    appliedModes += "+";
                            }
                            appliedModes += "o";
                        }
                        else
                        {
                            channel.removeOperator(targetFd);
                            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                            {
                                appliedModes += "-";
                            }
                            appliedModes += "o";
                        }
                        if (!appliedParams.empty()) appliedParams += " ";
                        appliedParams += targetNick;
                    }
                }
                else
                {
                    sendError(fd, 401, parsed, nick, "");
                }
            }
        }
    }

    if (!appliedModes.empty())
    {
        std::string modeBroadcast = ":" + nick + "!" + user + "@localhost MODE " + channelName + " " + appliedModes;
        if (!appliedParams.empty())
            modeBroadcast += " " + appliedParams;
        broadcastToChannel(modeBroadcast, channel);
    }
}

void handleNotice(int senderFd, const ParsedMessage& parsed, std::map<int, Client>& clientsByFd, 
                    std::map<std::string, int>& fdByNickUp, std::map<std::string, Channel>& channels)
{
    std::string host = "localhost";
    std::string senderNick = clientsByFd[senderFd].getNickname();

    if(parsed.params.size() < 2)
        return;

    std::string receiver = parsed.params[0];
    if(receiver[0] == '#')
    {
        std::map<std::string, Channel>::iterator it = channels.find(receiver);
        if (it == channels.end())
            return;

        Channel& chan = it->second;
        if(chan.hasUser(senderFd))
        {
            std::string msg = std::string(":") + senderNick + std::string("!") 
                            + clientsByFd[senderFd].getUsername() + "@" +  host 
                            + " NOTICE "+ receiver + std::string(" :") + parsed.params[1];
            sendToChannel(senderFd, msg, chan);
        }
    }
    else
    {
        std::map<std::string, int>::iterator it = fdByNickUp.find(toUpper(receiver));
        if(it == fdByNickUp.end())
            return;
        
        int receiverFd = it->second;
        std::string msg = std::string(":") + senderNick + std::string("!") 
                        + clientsByFd[senderFd].getUsername() + "@" +  host 
                        + " NOTICE "+ receiver + std::string(" :") + parsed.params[1];
        sendMsg(receiverFd, msg);
    }
}


static bool isKnownCommand(const std::string& cmd)
{
    return cmd == "PASS" || cmd == "NICK" || cmd == "USER" ||
           cmd == "CAP" || cmd == "PING" || cmd == "QUIT" ||
           cmd == "PRIVMSG" || cmd == "JOIN" || cmd == "PART" ||
           cmd == "KICK" || cmd == "INVITE" || cmd == "TOPIC" ||
           cmd == "MODE" || cmd == "NOTICE";
}

static bool isAllowedBeforeRegistration(const std::string& cmd)
{
    return cmd == "PASS" || cmd == "NICK" || cmd == "USER" ||
           cmd == "CAP" || cmd == "PING" || cmd == "QUIT";
}

int handleCommand(int fd, ParsedMessage parsed, std::map<int, Client>& clientsByFd, 
                std::map<std::string, int>& fdByNickUp, const std::string& serverPass, 
                std::map<std::string, Channel>& channels)
{
    Client& client = clientsByFd[fd];

    if (!isKnownCommand(parsed.command))
    {
        sendError(fd, 421, parsed, client.getNickname(), "");
        return 1;
    }

    if (!client.getRegistered() && !isAllowedBeforeRegistration(parsed.command))
    {
        sendError(fd, 451, parsed, client.getNickname(), "");
        return 1;
    }
    if (parsed.command == "PING")
    {
        handlePing(fd, parsed);
    }        
    else if (parsed.command == "NICK")
    {
        handleNick(fd, parsed, clientsByFd, fdByNickUp);
        //debugPrintClients(clientsByFd);
    }        
    else if (parsed.command == "PASS")
    {
        if(!handlePass(fd, parsed, clientsByFd, serverPass))
            return 0;
        //debugPrintClients(clientsByFd);
    }
    else if (parsed.command == "USER")
    {
        handleUser(fd, parsed, clientsByFd);
        //debugPrintClients(clientsByFd);
    }
    else if (parsed.command == "CAP")
    {        
        handleCap(fd, parsed);
    }
    else if (parsed.command == "PRIVMSG")
    {
        handlePrivmsg(fd, parsed, clientsByFd, fdByNickUp, channels);
    }
    else if(parsed.command == "JOIN")
    {
        handleJoin(fd, parsed, clientsByFd, channels);
    }
    else if (parsed.command == "PART")
    {
        handlePart(fd, parsed, clientsByFd, channels);
    }
    else if (parsed.command == "QUIT")
    {
        return handleQuit(fd, parsed, clientsByFd, channels);
    }
    else if (parsed.command == "KICK")
    {
        handleKick(fd, parsed, clientsByFd, fdByNickUp, channels);
    }
    else if (parsed.command == "INVITE")
    {
        handleInvite(fd, parsed, clientsByFd, fdByNickUp, channels);
    }
    else if (parsed.command == "TOPIC")
    {
        handleTopic(fd, parsed, clientsByFd, channels);
    }
    else if (parsed.command == "MODE")
    {
        handleMode(fd, parsed, clientsByFd, fdByNickUp, channels);
    }
    else if (parsed.command == "NOTICE")
    {
        handleNotice(fd, parsed, clientsByFd, fdByNickUp, channels);
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


GENERAL RULES
If new client types anything different before  
I should accept USER and NICK before PASS but I won't register before all 3 are valid
-----------------------------------------------
PASS:

if not registered and pass is wrong then: error 464 and close link
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
it ignores extra params, keeps the 1st param only as nickname

----------------------------------------------
USER:
USER <username> 0 * :<realname>
fields 0 and * are mostly historical leftovers. IRC ignores params 1 and 2.

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


----------------------------------------------
PRIVMSG:
PRIVMSG <target> :<message>
-if no parameters -> error 461
-if only 1 parameter -> error 412
-if more parameters -> accept only the first 2 and ignore the rest
-if target doesn't exist -> errors 401/403


*/


// gdb ./ircserv
// break handleNick
// run 6667 pass
//