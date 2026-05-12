/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: didimitr <didimitr@student.42luxembourg    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 12:13:44 by didimitr          #+#    #+#             */
/*   Updated: 2026/05/12 14:53:28 by didimitr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <map>
#include <set>
#include "parsedMessage.hpp"
#include "client.hpp"
#include <iostream>

class Channel
{
    private:
        std::string name;
        std::string topic;
        std::set<int> users;
        std::set<int> operators;
        std::string modes;
        std::string key;
        int userLimit;
        
    public:
        Channel();
        Channel(const std::string& n);
        // getters
        const std::string& getName() const;
        const std::string& getTopic() const;
        const std::set<int>& getUsers() const;
        bool isOperator(int fd) const;
        bool hasMode(char mode) const;
        // setters
        void setTopic(const std::string& t);
        void addMode(char mode);
        void removeMode(char mode);
        void setKey(const std::string& k);
        void setUserLimit(int limit);
        
        void addUser(int fd);
        void removeUser(int fd);
        void addOperator(int fd);
        void removeOperator(int fd);
        bool hasUser(int fd) const;
};

