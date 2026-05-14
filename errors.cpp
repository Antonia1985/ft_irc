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
            + nickname + std::string(" ") + channel
            + std::string(" :No such nick");
    }
    else if (error == 403)
    {
        msg = std::string(":ircserv 403 ") 
            + nickname + std::string(" ") + channel
            + std::string(" :No such channel");
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
    else 
    {
        msg = std::string(":ircserv unknown error");        
    }
    sendMsg(clientFd, msg);
}


void sendNotification(int clientFd, int notice, ParsedMessage parsed, std::string nickname, std::string channel)
{
    (void)parsed;
    std::string msg;
   
    if (nickname.empty())
        nickname = "*";

    if (notice == 1)
    {
        msg = std::string(":ircserv 001 ") 
            + nickname + std::string(" ") 
            + std::string(" :Welcome to the IRC Network ")
            + nickname;
    }
    else if (notice == 332)
    {
        msg = std::string(":ircserv 332 ") 
            + nickname + std::string(" ") + channel 
            + std::string(" :General discussion");
    }
    else if (notice == 353)
    {
        msg = std::string(":ircserv 353 ") 
            + nickname + std::string(" = ") + channel + std::string(" ");
            //+ here we add the channe members?????
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
| `401` | `ERR_NOSUCHNICK`        | No such nickname             | `:ircserv 401 George Bob :No such nick`         |
| `403` | `ERR_NOSUCHCHANNEL`     | Channel does not exist       | `:ircserv 403 George #test :No such channel`            |
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