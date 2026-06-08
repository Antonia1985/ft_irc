/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: didimitr <didimitr@student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 12:13:49 by didimitr          #+#    #+#             */
/*   Updated: 2026/05/12 14:54:29 by didimitr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"
#include <sstream>

Channel::Channel() 
    : name(""), 
      topic(""), 
      key(""),
      userLimit(-1)
{
}
// ...existing code...

void Channel::addOperator(int fd) {
    operators.insert(fd);
}

void Channel::removeOperator(int fd) {
    operators.erase(fd);
}

bool Channel::isOperator(int fd) const {
    return operators.find(fd) != operators.end();
}

void Channel::removeUser(int fd) {
    users.erase(fd);
}

bool Channel::hasUser(int fd) const {
    return users.find(fd) != users.end();
}

const std::string& Channel::getName() const {
    return name;
}

const std::string& Channel::getTopic() const {
    return topic;
}

const std::set<int>& Channel::getUsers() const {
    return users;
}

bool Channel::hasMode(char mode) const {
    return modes.find(mode) != std::string::npos;
}

void Channel::setTopic(const std::string& t) {
    topic = t;
}

void Channel::addMode(char mode) {
    if (!hasMode(mode)) {
        modes += mode;
    }
}

void Channel::removeMode(char mode) {
    size_t pos = modes.find(mode);
    if (pos != std::string::npos) {
        modes.erase(pos, 1);
    }
}

void Channel::setKey(const std::string& k) {
    key = k;
}

void Channel::setUserLimit(int limit) {
    userLimit = limit;
}

Channel::Channel(const std::string& n) 
    : name(n), 
      topic(""), 
      key(""),
      userLimit(-1)
{
}
void Channel::addUser(int fd) {
    this->users.insert(fd);
}

const std::string& Channel::getKey() const {
    return key;
}

int Channel::getUserLimit() const {
    return userLimit;
}

std::string Channel::getModesString() const {
    return "+" + modes;
}

std::string Channel::getModesParamsString() const {
    std::string params = "";
    for (size_t i = 0; i < modes.length(); ++i) {
        if (modes[i] == 'k') {
            if (!params.empty()) params += " ";
            params += key;
        } else if (modes[i] == 'l') {
            if (!params.empty()) params += " ";
            std::stringstream ss;
            ss << userLimit;
            params += ss.str();
        }
    }
    return params;
}

void Channel::inviteUser(const std::string& nickUp) {
    invitedUsers.insert(nickUp);
}

void Channel::uninviteUser(const std::string& nickUp) {
    invitedUsers.erase(nickUp);
}

bool Channel::isInvited(const std::string& nickUp) const {
    return invitedUsers.find(nickUp) != invitedUsers.end();
}

void Channel::clearInvites() {
    invitedUsers.clear();
}