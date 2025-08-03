/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 12:52:10 by sacha             #+#    #+#             */
/*   Updated: 2025/07/30 15:40:52 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Bot.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

Bot::Bot(const std::string& ip, const std::string& port, const std::string& pass,
        const std::string& nick, const std::string& user, const std::string& realname)
{
    _serverIp = ip;
    _serverPort = port;
    _password = pass;
    _nickname = nick;
    _username = user;
    _realname = realname;
}

Bot::~Bot()
{
    if (_socketFd >= 0)
        close(_socketFd);
}

bool Bot::connectToServer()
{
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0)
        return false;

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(std::stoi(_serverPort));
    
    if (inet_pton(AF_INET, _serverIp.c_str(), &serverAddr.sin_addr) <= 0)
        return false;
    
    if (connect(_socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
        return false;

    std::string auth = "PASS " + _password + "\r\n"
                    + "NICK " + _nickname + "\r\n"
                    + "USER " + _username + " 0 * :" + _realname + "\r\n";
    send(_socketFd, auth.c_str(), auth.length(), 0);
    return true;
}

void Bot::loadBadWords(const std::string& filename)
{
    std::ifstream file(filename.c_str());
    std::string word;
    while (std::getline(file, word)) {
        if (!word.empty())
            _badWords.insert(word);
    }
}

ParsedMessage Bot::parseIrcMessage(const std::string& raw)
{
    ParsedMessage result;
    std::string msg = raw;

    if (msg[0] == ':') {
        size_t excl = msg.find('!');
        if (excl != std::string::npos) {
            std::string potentialUser = msg.substr(1, excl - 1);
            // Vérifier que le nickname n'est pas vide
            if (!potentialUser.empty() && potentialUser != "*") {
                result.user = potentialUser;
            }
            size_t space = msg.find(' ');
            if (space != std::string::npos)
                msg = msg.substr(space + 1);
        }
    }

    size_t space = msg.find(' ');
    if (space != std::string::npos) {
        result.command = msg.substr(0, space);
        msg = msg.substr(space + 1);
    }

    if (!msg.empty() && msg[0] == '#') {
        space = msg.find(' ');
        if (space != std::string::npos) {
            result.channel = msg.substr(0, space);
            msg = msg.substr(space + 1);
        } else {
            result.channel = msg;
            msg = "";
        }
    }

    if (!msg.empty() && msg[0] == ':')
        result.message = msg.substr(1);
    else if (!msg.empty())
        result.message = msg;

    return result;
}

void Bot::handleServerMessage(const std::string& msg)
{
    ParsedMessage pm = parseIrcMessage(msg);

    // Message de bienvenue (001) - Bot connecté
    if (msg.find(" 001 ") != std::string::npos) {
        std::string joinCmd = "JOIN #bot\r\n";
        send(_socketFd, joinCmd.c_str(), joinCmd.length(), 0);
        sendMessage("#bot", "👮 Bot modérateur actif. Tapez !help pour voir les commandes disponibles.");
    }

    // Commande !help
    if (pm.command == "PRIVMSG" && pm.channel == "#bot") {
        // Enlever les espaces au début et à la fin du message
        std::string trimmedMsg = pm.message;
        while (!trimmedMsg.empty() && isspace(trimmedMsg[0]))
            trimmedMsg.erase(0, 1);
        while (!trimmedMsg.empty() && isspace(trimmedMsg[trimmedMsg.size() - 1]))
            trimmedMsg.pop_back();

        if (trimmedMsg == "!help") {
            sendMessage("#bot", "📋 Guide du Bot Modérateur :");
            sendMessage("#bot", "- Je surveille le langage utilisé dans le canal");
            sendMessage("#bot", "- Premier mot interdit = ⚠️ avertissement");
            sendMessage("#bot", "- Deuxième mot interdit = 🚫 expulsion du canal");
            sendMessage("#bot", "- Les avertissements sont comptés par utilisateur");
            sendMessage("#bot", "- La liste des mots interdits est configurable");
            return;
        }

        // Modération des messages
        for (std::set<std::string>::const_iterator it = _badWords.begin(); it != _badWords.end(); ++it) {
            if (pm.message.find(*it) != std::string::npos) {
                // Si l'utilisateur n'a pas de nom, utiliser une valeur par défaut
                std::string username = pm.user.empty() ? "Utilisateur" : pm.user;
                _warnings[username]++;
                
                if (_warnings[username] == 1) {
                    sendMessage("#bot", username + ": ⚠️ Premier avertissement! Le mot \"" + *it + "\" n'est pas autorisé.");
                } else if (_warnings[username] >= 2) {
                    sendMessage("#bot", "👮 " + username + " a été kick pour langage inapproprié répété.");
                    kickUser("#bot", username, "Language inapproprié après avertissement");
                    _warnings[username] = 0;
                }
                break;
            }
        }
    }
}

void Bot::sendMessage(const std::string& channel, const std::string& message)
{
    std::string msg = "PRIVMSG " + channel + " :" + message + "\r\n";
    send(_socketFd, msg.c_str(), msg.size(), 0);
}

void Bot::kickUser(const std::string& channel, const std::string& user, const std::string& reason)
{
    std::string msg = "KICK " + channel + " " + user + " :" + reason + "\r\n";
    send(_socketFd, msg.c_str(), msg.size(), 0);
}

void Bot::listenToServer()
{
    char buffer[512];
    int bytesRead;

    while (true) {
        bytesRead = recv(_socketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0)
            break;
        buffer[bytesRead] = '\0';
        handleServerMessage(buffer);
    }
}
