/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 12:52:07 by sacha             #+#    #+#             */
/*   Updated: 2025/07/29 12:52:07 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>

struct ParsedMessage {
    std::string user;
    std::string command;
    std::string channel;
    std::string message;
};

class Bot {
private:    
    int _socketFd;
    std::string _serverIp;
    std::string _serverPort;
    std::string _password;
    std::string _nickname;
    std::string _username;
    std::string _realname;

    std::set<std::string> _badWords;
    std::map<std::string, int> _warnings;

    ParsedMessage parseIrcMessage(const std::string& raw);
    void sendMessage(const std::string& channel, const std::string& message);
    void kickUser(const std::string& channel, const std::string& user, const std::string& reason);

public:
    Bot(const std::string& ip, const std::string& port, const std::string& pass,
        const std::string& nick, const std::string& user, const std::string& realname);
    ~Bot();

    bool connectToServer();
    void loadBadWords(const std::string& filename);
    void handleServerMessage(const std::string& msg);
    void listenToServer();
};

#endif