#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>

class Client
{
    private:
        int fd;
        std::string buffer;
        std::string nickname;
        std::string nicknameToUp;
        std::string username;
        bool passOk;
        bool registered;
    public:
        Client();

        int getFd() const;
        const std::string& getBuffer() const;
        const std::string& getNickname() const;
        const std::string& getNicknameToUp() const;
        const std::string& getUsername() const;
        bool getPassOk() const;
        bool getRegistered() const;

        void setFd(int f);
        void setBuffer(const std::string& buf);
        void setNickname(const std::string& nicknm);
        void setNicknameToUp(const std::string& nicknameToUp);
        void setUsername(const std::string& usernm) ;
        void setPassOk(bool p);
        void setRegistered(bool r);

        void appendToBuffer(const std::string& data);
        void appendToBuffer(const char* data, size_t len);
        void eraseFromBuffer(size_t pos, size_t len);

};

void removeClient(std::vector<pollfd>::iterator it, 
                std::map<int, Client>& clients, std::vector<pollfd>& fds, std::map<std::string, int>& fdByNickUp);

#endif