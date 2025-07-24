#include "Bot.hpp"

// ====================== UTILS ======================
// ###################################################
// #                UTILS FUNCTIONS                #
// ###################################################

void Bot::sendMessageToChannel(const std::string &target, const std::string &message) {
    sendCommand("PRIVMSG " + target + " :" + message + "\r\n");
}

void Bot::sendCommand(const std::string &command) {
    if (send(_serSocketBot, command.c_str(), command.size(), 0) == -1) {
        throw std::runtime_error("Failed to send command: " + command);
    }
}

void Bot::loadQuotes(const std::string &filePath) {
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file at path: " << filePath << std::endl;
        return ;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            _quotes.push_back(line);
        }
    }
    file.close();
    if (_quotes.empty()) {
        return ;
    }
    std::cout << "Loaded " << _quotes.size() << " quotes from " << filePath << std::endl;
}

void Bot::sendRandomQuote(const std::string &channel) {
    if (_quotes.empty()) {
        std::cerr << "No quotes loaded to send." << std::endl;
        return;
    }
    std::srand(std::time(0));
    int randomIndex = std::rand() % _quotes.size();
    std::string message = _quotes[randomIndex];

    sendMessageToChannel(channel, message);
}

// ====================== IMPORTANT FUNCTIONS ======================
// ###################################################
// #          IMPORTANT BOT LOGIC FUNCTIONS         #
// ###################################################


void Bot::handleCommand(const std::string &message) {
    size_t privmsgPos = message.find("PRIVMSG");
    if (privmsgPos != std::string::npos) {
        size_t channelStart = message.find(' ', privmsgPos) + 1;
        size_t channelEnd = message.find(' ', channelStart);
        std::string channel = message.substr(channelStart, channelEnd - channelStart);

        size_t messageStart = message.find(":", channelEnd) + 1;
        std::string msgContent = message.substr(messageStart);
        if (msgContent.find("!quotes") == 0) {
            sendRandomQuote(channel);
        }
    }
       if (message.find("PING") == 0) {
            std::string pongResponse = "PONG " + message.substr(5) + "\r\n";
            sendCommand(pongResponse);
        }
}

void Bot::listenToServer() {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(_serSocketBot, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            std::cerr << "Connection closed by server or error occurred." << std::endl;
            break;
        }
        std::string message(buffer);
        handleCommand(message);
    }
}

void Bot::joinServer() {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_port);

    _serSocketBot = socket(AF_INET, SOCK_STREAM, 0);
    if (_serSocketBot == -1) {
        throw std::runtime_error("Socket creation failed");
    }
    if (inet_pton(AF_INET, _ip.c_str(), &serverAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid IP address: " + _ip);
    }

    if (connect(_serSocketBot, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection error");
        throw std::runtime_error("Connection to server failed");
    }

    std::cout << "Connected to server: " << _ip << " on port " << _port << std::endl;
    _isConnected = true;
    sendCommand("PASS " + _password + "\r\n");
    sendCommand("NICK " + _nick + "\r\n");
    sendCommand("USER " + _user + " 0 * :" + _user + "\r\n");
}

void Bot::botInit() {
    try {
    std::cout << "Bot initialized with NICK: " << _nick << " and USER: " << _user << std::endl;
    joinServer();
    std::string channel = "#IA";
    sendCommand("PING\r\n");

    sendCommand("JOIN " + channel + "\r\n");
    std::cout << "Bot has joined channel " << channel << std::endl;
    std::string welcomeMessage = "Hello, I'm FiceloBot, and I'm here to help!";
    sendMessageToChannel(channel, welcomeMessage);
    }
    catch(std::exception &e){
        std::cout << e.what() << std::endl;
    }
}