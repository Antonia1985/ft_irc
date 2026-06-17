#pragma once

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <set>
class Channel;
#include "errors.hpp"

class Client
{
    private:
        int fd;
        std::string buffer;
        std::string nickname;
        std::string nicknameToUp;
        std::string username;
        std::string realname;
        bool passOk;
        bool registered;
        std::set<std::string> channels;
    public:
        Client();

        int getFd() const;
        const std::string& getBuffer() const;
        const std::string& getNickname() const;
        const std::string& getNicknameToUp() const;
        const std::string& getUsername() const;
        const std::string& getRealname() const;
        bool getPassOk() const;
        bool getRegistered() const;

        void setFd(int f);
        void setBuffer(const std::string& buf);
        void setNickname(const std::string& nicknm);
        void setNicknameToUp(const std::string& nicknameToUp);
        void setUsername(const std::string& usernm);
        void setRealname(const std::string& realnm);
        void setPassOk(bool p);
        void setRegistered(bool r);

        void appendToBuffer(const std::string& data);
        void appendToBuffer(const char* data, size_t len);
        void eraseFromBuffer(size_t pos, size_t len);
        //Channel
        void addChannel(std::string& channelName);
        void removeChannel(std::string& channelName);
        const std::set<std::string>& getChannels() const;

};
