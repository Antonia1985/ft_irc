#include "errors.hpp"
#include "commandHandler.hpp"
#include "client.hpp"

void sendError(int clientFd, int error, ParsedMessage parsed, std::string nickname, std::string channel)
{
    std::string msg;
   
    if (nickname.empty())
        nickname = "*";

    if (error == 401)
    {
        msg = std::string(":ircserv 401 ") 
            + nickname + std::string(" ") + parsed.params[0]
            + std::string(" :No such nick");
    }
    else if (error == 403)
    {
        msg = std::string(":ircserv 403 ") 
            + nickname + std::string(" ") + channel
            + std::string(" :No such channel");
    }
    else if (error == 412)
    {
        msg = std::string(":ircserv 412 ") 
            + nickname
            + std::string(" :No text to send");
    }
    else if (error == 421)
    {
        msg = std::string(":ircserv 421 ") 
            + nickname + std::string(" ") + parsed.command
            + std::string(" :Unknown command");
    }
    else if (error == 431)
    {
        msg = std::string(":ircserv 431 ") + nickname 
            + std::string(" :No nickname given");        
    }
    else if (error == 432)
    {
        msg = std::string(":ircserv 432 ")
            + nickname + std::string(" ")  + parsed.params[0]
            + std::string(" :Erroneous nickname");
        
    }
    else if (error == 433)
    {
        msg = std::string(":ircserv 433 ") 
            + nickname + std::string(" ")  + parsed.params[0]
            + std::string(" :Nickname is already in use");
        
    }
    else if (error == 442)
    {
        msg = std::string(":ircserv 442 ") 
            + nickname + std::string(" ") + channel
            + std::string(" :You're not on that channel");
    }
    else if (error == 451)
    {
        msg = std::string(":ircserv 451 ") + nickname 
            + std::string(" :You have not registered");        
    }
    else if (error == 461)
    {
        msg = std::string(":ircserv 461 ")
        + nickname + std::string(" ") + parsed.command
        + std::string(" :Not enough parameters");        
    }
    else if (error == 462)
    {
        msg = std::string(":ircserv 462 ") + nickname 
            + std::string(" :You may not reregister");
        
    }
    else if (error == 464)
    {
        msg = std::string(":ircserv 464 ") + nickname + std::string(" :Password incorrect");        
    }
    else if (error == 441)
    {
        std::string target = (parsed.params.size() > 1) ? parsed.params[1] : (parsed.params.size() > 0 ? parsed.params[0] : "user");
        msg = std::string(":ircserv 441 ") + nickname + " " + target + " " + channel + " :They aren't on that channel";
    }
    else if (error == 443)
    {
        std::string target = (parsed.params.size() > 0) ? parsed.params[0] : "user";
        msg = std::string(":ircserv 443 ") + nickname + " " + target + " " + channel + " :is already on channel";
    }
    else if (error == 471)
    {
        msg = std::string(":ircserv 471 ") + nickname + " " + channel + " :Cannot join channel (+l)";
    }
    else if (error == 473)
    {
        msg = std::string(":ircserv 473 ") + nickname + " " + channel + " :Cannot join channel (+i)";
    }
    else if (error == 475)
    {
        msg = std::string(":ircserv 475 ") + nickname + " " + channel + " :Cannot join channel (+k)";
    }
    else if (error == 482)
    {
        msg = std::string(":ircserv 482 ") + nickname + " " + channel + " :You're not channel operator";
    }
    else 
    {
        msg = std::string(":ircserv unknown error");        
    }
    sendMsg(clientFd, msg);
}


void sendNotification(int clientFd, int notice, ParsedMessage parsed, std::string nickname, std::string channel)
{
    std::string msg;
   
    if (nickname.empty())
        nickname = "*";

    if (notice == 1)
    {
        sendMsg(clientFd, std::string(":ircserv 001 ") + nickname + " :Welcome to the IRC Network " + nickname);
        sendMsg(clientFd, std::string(":ircserv 002 ") + nickname + " :Your host is ircserv, running version 1.0");
        sendMsg(clientFd, std::string(":ircserv 003 ") + nickname + " :This server is running ft_irc");
        sendMsg(clientFd, std::string(":ircserv 004 ") + nickname + " ircserv 1.0 o itkol");
        return;
    }
    else if (notice == 324)
    {
        std::string modes = (parsed.params.size() > 0) ? parsed.params[0] : "";
        std::string params = (parsed.params.size() > 1) ? parsed.params[1] : "";
        msg = std::string(":ircserv 324 ") + nickname + " " + channel + " " + modes;
        if (!params.empty())
            msg += " " + params;
    }
    else if (notice == 331)
    {
        msg = std::string(":ircserv 331 ") + nickname + " " + channel + " :No topic is set";
    }
    else if (notice == 332)
    {
        std::string topic = (parsed.params.size() > 0) ? parsed.params[0] : "General discussion";
        msg = std::string(":ircserv 332 ") + nickname + " " + channel + " :" + topic;
    }
    else if (notice == 341)
    {
        std::string target = (parsed.params.size() > 0) ? parsed.params[0] : "";
        msg = std::string(":ircserv 341 ") + nickname + " " + target + " " + channel;
    }
    else if (notice == 353)
    {
        std::string userList = (parsed.params.size() > 0) ? parsed.params[0] : "";
        msg = std::string(":ircserv 353 ") + nickname + " = " + channel + " :" + userList;
    }
    else if (notice == 366)
    {
        msg = std::string(":ircserv 366 ") 
            + nickname + std::string(" ") + channel
            + std::string(" :End of /NAMES list");
    }
    sendMsg(clientFd, msg);
}

/*
(!!! the * is basically: "nickname placeholder !!!)


------------------ erors -------------------

| Code  | Name                    | Meaning                      | Example reply                                           |
| ----- | ----------------------- | ---------------------------- | ------------------------------------------------------- |
| `401` | `ERR_NOSUCHNICK`        | No such nickname             | `:ircserv 401 George Bob :No such nick`                 |
| `403` | `ERR_NOSUCHCHANNEL`     | Channel does not exist       | `:ircserv 403 George #test :No such channel`            |
| `412` | `ERR_NOTEXTTOSEND`      | User tried to send PRIVMSG/  | `:ircserv 412 George :No text to send`                         |
|                                 |  NOTICE without a messg body |                                                         |
| `421` | `ERR_UNKNOWNCOMMAND`    | Unknown command              | `:ircserv 421 George ABC :Unknown command`              |
| `431` | `ERR_NONICKNAMEGIVEN`   | Missing nickname             | `:ircserv 431 * :No nickname given`                     |
| `432` | `ERR_ERRONEUSNICKNAME`  | Invalid nickname             | `:ircserv 432 * @@@ :Erroneous nickname`                |
| `433` | `ERR_NICKNAMEINUSE`     | Nickname already used        | `:ircserv 433 * George :Nickname is already in use`     |
| `442` | `ERR_NOTONCHANNEL`      | User not in channel          | `:ircserv 442 George #test :You're not on that channel` |
| `451` | `ERR_NOTREGISTERED`     | Client not authenticated yet | `:ircserv 451 * :You have not registered`               |
| `461` | `ERR_NEEDMOREPARAMS`    | Missing command parameters   | `:ircserv 461 George JOIN :Not enough parameters`       |
| `462` | `ERR_ALREADYREGISTERED` | USER sent twice              | `:ircserv 462 George :You may not reregister`           |
| `464` | `ERR_PASSWDMISMATCH`    | Wrong password               | `:ircserv 464 * :Password incorrect`                    |




-------------- replies / notifications ------------

| Code  | Name             | Meaning                        | Example reply                                            |
| ----- | ---------------- | ------------------------------ | -------------------------------------------------------- |
| `001` | `RPL_WELCOME`    | Client successfully registered | `:ircserv 001 George :Welcome to the IRC Network George` |
| `332` | `RPL_TOPIC`      | Sends channel topic            | `:ircserv 332 George #chat :General discussion`          |
| `353` | `RPL_NAMREPLY`   | Sends users in channel         | `:ircserv 353 George = #chat :George Alice Bob`          |
| `366` | `RPL_ENDOFNAMES` | End of names list              | `:ircserv 366 George #chat :End of /NAMES list`          |

*/
