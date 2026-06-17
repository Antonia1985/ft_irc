#include "client.hpp"

Client::Client()
    : fd(-1),
      buffer(""),
      nickname(""),
      nicknameToUp(""),      
      username(""),
      realname(""),
      passOk(false),
      registered(false)
{
}

int Client::getFd() const { return fd; }
const std::string& Client::getBuffer() const { return buffer; }
const std::string& Client::getNickname() const { return nickname; }
const std::string& Client::getNicknameToUp() const { return nicknameToUp; }
const std::string& Client::getUsername() const { return username; }
const std::string& Client::getRealname() const { return realname; }
bool Client::getPassOk() const { return passOk; }
bool Client::getRegistered() const { return registered; }

void Client::setFd(int f) { fd = f; }
void Client::setBuffer(const std::string& buf) { buffer = buf; }
void Client::setNickname(const std::string& nicknm) { nickname = nicknm; }
void Client::setNicknameToUp(const std::string& nicknmUp) { nicknameToUp = nicknmUp; }
void Client::setUsername(const std::string& usernm) { username = usernm; }
void Client::setRealname(const std::string& realnm) { realname = realnm; }
void Client::setPassOk(bool p) { passOk = p; }
void Client::setRegistered(bool r) { registered = r; }

void Client::appendToBuffer(const std::string& data)
{
    buffer += data;
}

void Client::appendToBuffer(const char* data, size_t len)
{
    buffer.append(data, len);
}


void Client::eraseFromBuffer(size_t pos, size_t len)
{
    buffer.erase(pos, len);
}

void Client::addChannel(std::string& channelName) {
    this->channels.insert(channelName);
}

void Client::removeChannel(std::string& channelName) {
    this->channels.erase(channelName);
}

const std::set<std::string>& Client::getChannels() const {
    return channels;
}
