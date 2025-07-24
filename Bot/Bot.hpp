#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <poll.h>
#include <map>
#include <stdlib.h>
#include <csignal>
#include <fstream>
#include <stdbool.h>
#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

#pragma once
class Channel;
class Client;
class Server;

class Bot : public Server {
private:
    std::vector<std::string> _quotes;
	std::string _ip;
    std::string _nick;
    std::string _user;
    int _serSocketBot;

public:
    Bot(int port, std::string ip, std::string nick, std::string user, std::string pass) : Server(port, pass), _ip(ip), _nick(nick), _user(user), _serSocketBot(-1) {}
    ~Bot() {closeConnection();}

    void botInit();
    void loadQuotes(const std::string &filePath);
    void sendRandomQuote(const std::string &channel);
    void handleCommand(const std::string &message);
    void sendMessageToChannel(const std::string &target, const std::string &message);
    void sendCommand(const std::string &command);
    void joinServer();
    void listenToServer();
    void closeConnection();
};
