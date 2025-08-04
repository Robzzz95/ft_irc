/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/08/04 17:40:12 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _socket(-1), _name("ft_irc"), _info("My first irc server")
{
}

Server::~Server()
{
	std::cout << YELLOW << "\nSERVER CLEANUP...\n" << RESET;
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
		delete it->second;
	_channels.clear();
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first >= 0)
			close(it->first);
		delete it->second;
	}
	_clients.clear();
	if (_socket >= 0)
		close(_socket);
	std::cout << GREEN << "\nSERVER CLEANUP COMPLETE" << RESET << std::endl;
}

int	Server::get_port() const
{
	return (_port);
}

std::string const &	Server::get_password() const
{
	return (_password);
}

void	Server::ftErrorServ(std::string const & str)
{
	if (_socket != -1)
		close(_socket);
	std::cerr << RED "Error: " RESET << str << std::endl;
	exit(1);
}

void	Server::initCommands()
{
	_commands["PASS"] = &Server::passCommand;
	_commands["NICK"] = &Server::nickCommand;
	_commands["USER"] = &Server::userCommand;
	_commands["PRIVMSG"] = &Server::privmsgCommand;
	_commands["QUIT"] = &Server::quitCommand;
	_commands["EXIT"] = &Server::quitCommand;
	_commands["PING"] = &Server::pingCommand;
	_commands["PONG"] = &Server::pingCommand;
	_commands["JOIN"] = &Server::joinCommand;
	_commands["CAP"] = &Server::capCommand;
	_commands["MODE"] = &Server::modeCommand; //tester les modes!
	_commands["WHOIS"] = &Server::whoisCommand; //a tesst
	_commands["PART"] = &Server::partCommand;
	_commands["KICK"] = &Server::kickCommand; //tester
	_commands["INVITE"] = &Server::inviteCommand;
	_commands["TOPIC"] = &Server::topicCommand;
	_commands["WHO"] = &Server::whoCommand; //test 
	_commands["NAMES"] = &Server::namesCommand;
	_commands["LIST"] = &Server::listCommand; //test
	// _commands["NOTICE"] = &Server::noticeCommand;
}

void	Server::initServ()
{
	setupSocket();
	initCommands();
	time_t lastTimeoutCheck = time(NULL);
	while (g_running)
	{
		int ret = poll(&_poll_fds[0], _poll_fds.size(), -1);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue ;
			std::cerr << "poll() failed." << std::endl;
			break ;
		}
		for (size_t i = 0; i < _poll_fds.size(); i++)
		{
			if (_poll_fds[i].revents & POLLIN)
			{
				if (_poll_fds[i].fd == _socket)
					acceptClient();
				else if (_clients.find(_poll_fds[i].fd) != _clients.end())
					handleClient(_poll_fds[i].fd);
			}
		}
		time_t now = time(NULL);
		if (now - lastTimeoutCheck >= TIMEOUT_CHECK)
		{
			checkClientTimeouts();
			lastTimeoutCheck = now;
		}
	}
}

void	Server::setupSocket()
{
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		ftErrorServ("socket() failed.");
	int opt = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		ftErrorServ("setsockopt() failed.");
	if (fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
		ftErrorServ("fcntl() failed.");

	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);

	if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		ftErrorServ("bind() failed.");
	if (listen(_socket, SOMAXCONN) < 0)
		ftErrorServ("listen() failed.");
	
	struct pollfd	pfd;
	pfd.fd = _socket;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_poll_fds.push_back(pfd);

	char	hostname[256];
	gethostname(hostname, sizeof(hostname));
	struct hostent* host = gethostbyname(hostname);
	struct in_addr** addr_list = (struct in_addr**)host->h_addr_list;
	std::cout << GREEN << "SERVER LISTENING ON " << RESET << inet_ntoa(*addr_list[0]) << ":" <<  _port << std::endl;
}

void	Server::acceptClient()
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int	client_fd = accept(_socket, (struct sockaddr*)&client_addr, &addr_len);
	
	if (client_fd < 0)
	{
		std::cerr << "Error accepting client." << std::endl;
		return ;
	}
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error: fcntl() failed" << std::endl;
		return ;
	}
	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_poll_fds.push_back(pfd);

	std::string ip = inet_ntoa(client_addr.sin_addr);
	std::string hostname = ip;
	_clients[client_fd] = new Client(client_fd, ip, hostname);
	std::cout << "Accepted new client on FD: " << client_fd << std::endl;
	std::cout << "	New clients ip: " << ip << ", its hostname is: " << hostname << std::endl;
	sendRawMessage(client_fd, ":ft_irc NOTICE * : Please enter your password using <PASS>\r\n"); //for netcat
}

void	Server::handleClient(int fd)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	ssize_t bytes_read = recv(fd, buffer, BUFFER_SIZE - 1, 0);
	if (bytes_read <= 0)
	{
		if (bytes_read == 0)
		{
			std::cout << "Client on FD " << fd << " disconnected." << std::endl;

			for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
			{
				if (it->fd == fd)
				{
					_poll_fds.erase(it);
					break;
				}
			}
			close(fd);
			delete _clients[fd];
			_clients.erase(fd);
		}
		return;
	}

	std::cout << "FROM CLIENT: " << buffer << std::endl;

	Client *client = _clients[fd];
	client->updateActivity();
	client->appendToBuffer(std::string(buffer, bytes_read));
	std::vector<std::string> lines = client->extractLines();
	for (size_t i = 0; i < lines.size(); i++)
	{
		if (_clients.find(fd) == _clients.end())
			break ;
		processCommand(fd, lines[i]);
	}
}

int	Server::processCommand(int fd, const std::string &line)
{
	std::vector<std::string> vec = splitIrc(line);
	if (vec.empty())
		return (0);
	std::map<std::string, commandHandler>::iterator it = _commands.find(vec[0]);
	if (it != _commands.end())
	{
		commandHandler	handler = it->second;
		(this->*handler)(fd, vec);
		if (_clients.find(fd) == _clients.end())
			return (1);
	}
	return (0);
}

void	Server::sendMessageFromServ(int fd, int code, const std::string &message)
{
	Client*	client = _clients[fd];
	std::ostringstream oss;

	oss << ":" << _name << " " \
		<< std::setw(3) << std::setfill('0') << code << " " \
		<< (client->getNickname().empty() ? "*" : client->getNickname()) << " :" \
		<< message << "\r\n";

	std::string str = oss.str();
	ssize_t sent = send(fd, str.c_str(), str.length(), 0);
	if (sent < 0)
		std::cerr << "Error : failed to send message to FD: " << fd << std::endl;
}

void	Server::sendRawMessage(int fd, const std::string &message)
{
	ssize_t sent = send(fd, message.c_str(), message.length(), 0);
	if (sent < 0)
		std::cerr << "Error: failed to send raw message." << std::endl;
}

void	Server::removeClientFromAllChannels(int fd)
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
	{
		Channel* channel = it->second;
		if (channel->hasClient(fd))
		{
			channel->removeClient(fd);			
			std::string partingMsg = ":" + _clients[fd]->getPrefix() + " PART " + channel->getName() + "\r\n";
			channel->broadcast(partingMsg, fd);
		}
	}
}

void	Server::closeConnection(int fd)
{
	for (std::vector<pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); it++)
	{
		if (it->fd == fd)
		{
			_poll_fds.erase(it);
			break ;
		}
	}
	close(fd);
	if (_clients.find(fd) != _clients.end())
	{
		removeClientFromAllChannels(fd);
		delete _clients[fd];
		_clients.erase(fd);
	}
	std::cout << "Closed connection on FD: " << fd << std::endl;
}

void	Server::passCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (!client)
		return ;
	if (client->getAuthentificated())
	{
		sendMessageFromServ(fd, 462, "Already authentificated.");
		return ;
	}
	if (vec.size() < 2)
	{
		sendMessageFromServ(fd, 464, "Error: PASS command takes only one argument.");
		return ;
	}
	if (vec[1] != _password)
	{
		sendMessageFromServ(fd, 464, "Error: wrong password.\nYou have been disconnected.");
		closeConnection(fd);
		return ;
	}
	client->setAuthentificated(true);
}

void	Server::nickCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];

	if (!client->getAuthentificated())
	{
		sendMessageFromServ(fd, 464, "Error: Password required.\r\n");
		closeConnection(fd);
		return ;
	}
	if (vec.size() != 2 || vec[1].empty())
	{
		sendMessageFromServ(fd, 464, "Error: NICK wrong parameters");
		if (!client->getRegistered())
			closeConnection(fd);
		return ; 
	}
	if (!isValidNickname(vec[1]))
	{
		sendMessageFromServ(fd, 432, "Error: invalid nickname.");
		if (!client->getRegistered())	
			closeConnection(fd);
		return ;
	}
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first != fd && it->second->getNickname() == vec[1])
		{
			sendMessageFromServ(fd, 433, "Nickname already in use.");
			if (!client->getRegistered())
				closeConnection(fd);
			return ; 
		}
	}
	std::string old_nick = client->getNickname();
	sendRawMessage(fd, (client->getPrefix() + " NICK :" + vec[1] + "\r\n"));
	client->setNickname(vec[1]);
	std::string msg = ":" + old_nick + "!" + client->getUsername() + "@" + client->getHostname() \
		+ " NICK :" + vec[1] + "\r\n";
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
	{
		if (it->second->hasClient(fd))
			it->second->broadcast(msg, fd);
	}
}

void	Server::userCommand(int fd, std::vector<std::string> vec)
{
	if (_clients.find(fd) == _clients.end())
		return ;
	Client*	client = _clients[fd];
	if (client->getRegistered())
	{
		sendMessageFromServ(fd, 462, "Error: You are already registered.");
		return ;
	}

	if (vec.size() < 5)
	{
		sendMessageFromServ(fd, 461, "USER :Not enough parameters.");
		return ;
	}
	std::string username = vec[1];
	std::string realname;
	for (size_t i = 4; i < vec.size(); i++)
	{
		if (!realname.empty())
			realname += " ";
		realname += vec[i];
	} 
	if (username.empty() || realname.empty())
	{
		sendMessageFromServ(fd, 461, "USER :Invalid username or realname.");
		return ;
	}
	size_t i = 0;
	while (realname[i] == ':' || realname[i] == ' ')
		i++;
	realname.erase(0, i);
	client->setUsername(username);
	client->setRealname(realname);
	client->setRegistered(true);
	sendMessageFromServ(fd, 001, client->getNickname() + " :Welcome to the IRC Network " + client->getPrefix());
	sendMessageFromServ(fd, 002, client->getNickname() + " :Your host is " + _name + ", running version v1.0");
	sendMessageFromServ(fd, 003, client->getNickname() + " :This server was created Mon Jun 10 2025 at 13:45:00");
	sendMessageFromServ(fd, 004, _name + " v1.0 o o");
}

void	Server::quitCommand(int fd, std::vector<std::string> vec)
{
	std::string reason = vec[1];
	std::string message;
	if (vec.size() > 2)
	{
		size_t i = 0;
		while (i < reason.size() && (reason[i] == ':' || reason[i] == ' '))
			i++;
		reason = reason.substr(i);
		if (!reason.empty())
			message = reason;
	}
	closeConnection(fd);
}

void	Server::privmsgCommand(int fd, std::vector<std::string> vec)
{
	Client*	sender = _clients[fd];
	if (!sender->getRegistered())
		return sendMessageFromServ(fd, 451, "You have not registered.");
	if (vec.size() < 2)
		return sendMessageFromServ(fd, 461, "PRIVMSG: not enough parameters.");
	
	std::string	recipient, message;
	recipient = vec[1];
	for (size_t i = 2; i < vec.size(); i++)
	{
		message += vec[i];
		if (i + 1 < vec.size())
			message += ' ';	
	}
	if (!message.empty() && message[0] == ':')
		message.erase(0, 1);
	if (recipient.empty() || message.empty())
		return sendMessageFromServ(fd, 461, "PRIVMSG: not enough parameters.");
	//message to channel
	if (recipient[0] == '#')
	{
		Channel* channel = getChannelByName(recipient);
		if (!channel)
			return sendMessageFromServ(fd, 403, recipient + " :No such channel");
		if (!channel->hasClient(fd))
			return sendMessageFromServ(fd, 404, recipient + " :Cannot send to channel");
		channel->broadcast(sender->getPrefix() + " PRIVMSG " + recipient + " :" + message + "\r\n", sender->getFd());
		return ;
	}
	//private message
	Client*	target = findClientByNickname(recipient);
	if (!target)
		return sendMessageFromServ(fd, 401, recipient + " :No such nick.");
	std::string full_message = sender->getPrefix() + " PRIVMSG " +  recipient + " :" + message + "\r\n";
	sendRawMessage(target->getFd(), full_message);
}

Client*	Server::findClientByNickname(const std::string &nickname)
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->getNickname() == nickname)
			return (it->second);
	}
	return (NULL);
}

Channel*	Server::getChannelByName(const std::string &str)
{
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
	{
		if (it->first == str)
			return (it->second);
	}
	return (NULL);
}

void	Server::pongCommand(int fd, std::vector<std::string> vec)
{
	(void)vec;
	Client* client = _clients[fd];
	client->updateActivity();
}

void	Server::pingCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	std::string token = (vec.size() >= 2) ? vec[1] : _name;
	client->setLastActivity(time(NULL));
	std::string msg = "PONG " + _name + " " + token + "\r\n";
	sendRawMessage(fd, msg);
}

void	Server::checkClientTimeouts()
{
	time_t now = time(NULL);
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		Client*	client = it->second;
		if (client->isAwaitingPong())
		{
			if (now - client->getLastPing() > PING_TIMEOUT)
			{
				sendRawMessage(client->getFd(), "ERROR :Ping timeout\r\n");
				closeConnection(client->getFd());
			}
		}
		else if (now - client->getLastActivity() > PING_CHECK)
		{
			std::stringstream ss;
			ss << "PING :" << now << "\r\n";
			client->setLastPing(now);
			client->setAwaitingPong(true);
			sendRawMessage(client->getFd(), ss.str());
		}
	}
}

void Server::joinCommand(int fd, std::vector<std::string> vec)
{
	Client* client = _clients[fd];
	if (!client->getRegistered())
		return sendMessageFromServ(fd, 451, "you must register first.");
	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, "JOIN: not enough parameters.");
	std::vector<std::string> channels = splitList(vec[1]);
	std::vector<std::string> keys;
	if (vec.size() >= 3)
		keys = splitList(vec[2]);

	for (size_t i = 0; i < channels.size(); i++)
	{
		std::string &channel_name = channels[i];
		std::string key = (i < keys.size()) ? keys[i] : "";
		if (channel_name.empty() || channel_name[0] != '#')
			return sendMessageFromServ(fd, 476, " :Bad Channel Mask");
		Channel* new_channel;
		if (_channels.find(channel_name) == _channels.end())
		{
			new_channel = new Channel(channel_name);
			_channels[channel_name] = new_channel;
		}
		else
			new_channel = _channels[channel_name];

		std::string msg;
		if (new_channel->isInviteOnly() && !new_channel->isInvited(fd))
		{
			msg = ":" + _name + " 471 " + client->getNickname() + " " + channel_name + \
				" :Cannot join channel (+i)\r\n";
			sendRawMessage(fd, msg);
			continue ;
		}
		if (new_channel->hasLimit() && static_cast<int>(new_channel->getClientList().size()) >= new_channel->getLimit())
    	{
			msg = ":" + _name + " 471 " + client->getNickname() + " " + channel_name + \
				" :Cannot join channel (+l)\r\n";
			sendRawMessage(fd, msg);
			continue ;
		}	
        if (new_channel->hasPassword() && key != new_channel->getPassword())
		{
			msg = ":" + _name + " 471 " + client->getNickname() + " " + channel_name + \
				" :Cannot join channel (+k)\r\n";
			sendRawMessage(fd, msg);
			continue ;
		}
		_channels[channel_name]->addClient(fd, client);

		std::string message = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN " + channel_name + "\r\n";
		_channels[channel_name]->broadcast(message, -1);
		sendRawMessage(fd, ":ft_irc 331 " + client->getNickname() + " " + channel_name + " :no topic is set.\r\n");

		Channel* channel = _channels[channel_name];
		std::string names;
		std::vector<Client*> clients = channel->getClientList();
		for (size_t j = 0; j < clients.size(); ++j)
		{
			if (channel->isOperator(clients[j]->getFd()))
				names += "@";
			names += clients[j]->getNickname();
				if (j + 1 < clients.size())
				names += " ";
		}
		std::string msg = ":" + _name + " 353 " + client->getNickname() + " = " + channel_name + " :" + names + "\r\n";
		sendRawMessage(fd, msg);
		msg.clear();
		msg = ":" + _name + " 366 " + client->getNickname() + " " + channel_name + " :End of NAMES list\r\n";
		sendRawMessage(fd, msg);
	}
}

void	Server::partCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, " PART Need more parameters");
	std::vector<std::string> channels = splitList(vec[1]);
	std::string reason;
	if (vec.size() > 2)
	{
		for (size_t i = 2; i < vec.size(); i++)
		{
			if (i > 2) reason += " ";
			reason += vec[i];
		}
		if (!reason.empty() && reason[0] == ':')
			reason.erase(0, 1);
	}
	for (size_t i = 0; i < channels.size(); i++)
	{
		std::string channel_name = channels[i];
		if (channel_name.empty() || channel_name[0] != '#')
			return sendMessageFromServ(fd, 476, channel_name + " :Invalid channel name.");
		Channel* channel_ptr = getChannelByName(channel_name);
		if (!channel_ptr)
			return sendMessageFromServ(fd, 403, channel_name + " :No such channel");
		if (!channel_ptr->hasClient(fd))
			return sendMessageFromServ(fd, 442, channel_name + " :You are not on that channel");
		std::string msg = ":" + client->getPrefix() + " PART " + channel_name;
		if (!reason.empty())
			msg += " :" + reason;
		msg += "\r\n";
		channel_ptr->broadcast(msg, -1);
		channel_ptr->removeClient(fd);
		if (channel_ptr->isEmpty())
		{
			delete channel_ptr;
			_channels.erase(channel_name);
		}
	}
}

void	Server::capCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 2)
		return sendMessageFromServ(fd, 461, " CAP :Need more parameters");
	std::string param = vec[1];
	if ((!client->getAuthentificated() && !client->getRegistered()) && param != "END")
	{
		std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
		sendRawMessage(fd, ":irc.yourserver.net CAP " + nick + " LS :\r\n");
	}
	else if (param == "END")
		return ;
}

void Server::modeCommand(int fd, std::vector<std::string> vec)
{
    Client* client = _clients[fd];
    if (vec.size() < 2) {
        sendMessageFromServ(fd, 461, "MODE :Not enough parameters");
        return;
    }

    std::string channelName = vec[1];
    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        // sendMessageFromServ(fd, 403, channelName + " :No such channel");
        return;
    }

    // Si seulement le nom du channel est donné, on affiche les modes actuels
    if (vec.size() == 2) {
        std::string modes = "+";
        std::string params;
		std::stringstream oss;
		oss << channel->getLimit();

        if (channel->isInviteOnly()) modes += "i";
        if (channel->isTopicLocked()) modes += "t";
        if (channel->hasPassword())  { modes += "k"; params += " " + channel->getPassword(); }
        // if (channel->hasLimit())     { modes += "l"; params += " " + std::to_string(channel->getLimit()); }
		if (channel->hasLimit())     { modes += "l"; params += " " + oss.str(); }
        sendMessageFromServ(fd, 324, channelName + " " + modes + params);
        return;
    }

    // Seul un opérateur peut changer les modes
    if (!channel->isOperator(fd)) {
        sendMessageFromServ(fd, 482, channelName + " :You're not channel operator");
        return;
    }

    std::string modeString = vec[2];
    size_t paramIndex = 3;
    bool adding = true;
    std::string modeChanges = ":" + client->getPrefix() + " MODE " + channelName + " " + modeString;

    for (size_t i = 0; i < modeString.length(); ++i) {
        char modeChar = modeString[i];
        if (modeChar == '+') adding = true;
        else if (modeChar == '-') adding = false;
        else if (modeChar == 'i') channel->setInviteOnly(adding);
        else if (modeChar == 't') channel->setTopicLocked(adding);
        else if (modeChar == 'k') {
            if (adding) {
                if (vec.size() <= paramIndex) {
                    sendMessageFromServ(fd, 461, "MODE :Not enough parameters for +k");
                    return;
                }
                channel->setPassword(vec[paramIndex]);
                channel->setHasPassword(true);
                paramIndex++;
            } else {
                channel->setHasPassword(false);
                channel->setPassword("");
            }
        }
        else if (modeChar == 'l') {
            if (adding) {
                if (vec.size() <= paramIndex) {
                    sendMessageFromServ(fd, 461, "MODE :Not enough parameters for +l");
                    return;
                }
                int limit = std::atoi(vec[paramIndex].c_str());
                channel->setLimit(limit);
                channel->setHasLimit(true);
                paramIndex++;
            } else {
                channel->setHasLimit(false);
                channel->setLimit(-1);
            }
        }
        else if (modeChar == 'o') {
            if (vec.size() <= paramIndex) {
                sendMessageFromServ(fd, 461, "MODE :Not enough parameters for +o/-o");
                return;
            }
            std::string nick = vec[paramIndex++];
            Client* target = findClientByNickname(nick);
            if (!target || !channel->hasClient(target->getFd())) {
                sendMessageFromServ(fd, 441, nick + " " + channelName + " :They aren't on that channel");
                return;
            }
            if (adding)
                channel->makeOperator(target->getFd());
            else
                channel->removeOperator(target->getFd());
        }
        else {
            sendMessageFromServ(fd, 472, std::string(1, modeChar) + " :is unknown mode char to me");
            return;
        }
    }

    // Notifier tous les membres du channel du changement de mode
    std::vector<Client*> clients = channel->getClientList();
    for (size_t i = 0; i < clients.size(); ++i) {
        sendRawMessage(clients[i]->getFd(), modeChanges + "\r\n");
    }
}

void	Server::whoisCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, " PART Need more parameters");
	std::string name = vec[1];
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (name == it->second->getNickname())
		{
			std::string msg = client->getNickname() + " " + name + " " + it->second->getUsername() + " " \
				+ it->second->getHostname() + " * :" + it->second->getRealname();  
			sendMessageFromServ(fd, 311, msg);
			msg = client->getNickname() + " " + it->second->getNickname() + " " + _name + " :" + _info;
			sendMessageFromServ(fd, 312, msg);
			msg = client->getNickname() + " " + it->second->getNickname() + " :End of WHOIS list";
			sendMessageFromServ(fd, 318, msg);
			return ;
		}
	}
	std::string message = client->getNickname() + " " + name + " :No such nick/channel";
	sendMessageFromServ(fd, 401, message);
}

void	Server::topicCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];

	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, "TOPIC :Not enough parameters");
	std::string channel_name = vec[1];
	Channel* channel = getChannelByName(channel_name);
	if (!channel)
		return sendMessageFromServ(fd, 403, channel_name + " :No such channel");
	if (!channel->hasClient(fd))
		return sendMessageFromServ(fd, 442, channel_name + " : You're not on that channel");
	if (vec.size() == 2)
	{
		if (channel->getTopic().empty())
			return sendMessageFromServ(fd, 331, channel_name + " :No topic is set");
		return sendMessageFromServ(fd, 332, channel_name + " :" + channel->getTopic());
	}
	if (channel->isTopicLocked() && !channel->isOperator(fd))
		return sendMessageFromServ(fd, 482, channel_name + " You're not channel operator");
	std::string new_topic = "";
	for (size_t i = 2; i < vec.size(); i++)
	{
		new_topic += vec[i];
		if (i + 1 < vec.size())
			new_topic += ' ';
	}
	if (!new_topic.empty() && vec[2][0] == ':')
		new_topic.erase(0, 1);
	channel->setTopic(new_topic);
	std::string msg = client->getPrefix() + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
	channel->broadcast(msg, -1);
}

void	Server::kickCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 3)
		return sendMessageFromServ(fd, 461, "KICK :Not enough parameters");
	std::string channel_name = vec[1];
	std::vector<std::string> target_list = splitList(vec[2]);
	std::string reason = "Kicked";
	Channel* channel = getChannelByName(channel_name);
	if (!channel)
		return sendMessageFromServ(fd, 403, channel_name + " :No such channel");
	if (!channel->hasClient(fd))
		return sendMessageFromServ(fd, 442, channel_name + " :You're not on that channel");
	if (!channel->isOperator(fd))
		return sendMessageFromServ(fd, 482, channel_name + " :You're not channel operator");
	if (vec.size() > 3)
	{
		reason.clear();
		for (size_t i = 3; i < vec.size(); i++)
		{
			reason += vec[i];
			if (i + 1 < vec.size())
				reason += " ";
		}
		if (!reason.empty() && reason[0] == ':')
			reason.erase(0, 1);
	}
	for (size_t i = 0; i < target_list.size(); i++)
	{
		Client* target = findClientByNickname(target_list[i]);
		if (!target || !channel->hasClient(target->getFd()))
		{
			sendMessageFromServ(fd, 441, target_list[i] + " " + channel_name + " They aren't on that channel");
			continue ;
		}
		std::string msg = client->getPrefix() + " KICK " + channel_name + " " + target->getNickname() + " :" + reason + "\r\n";
		channel->broadcast(msg, -1);
		channel->removeClient(target->getFd());
	}
}

void	Server::inviteCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 2)
		return sendMessageFromServ(fd, 461, "INVITE :Not enough parameters");
	Client*	target = findClientByNickname(vec[1]);
	if (!target)
		return sendMessageFromServ(fd, 401, vec[1] + " :No such nick");
	Channel* channel = getChannelByName(vec[2]);
	if (!channel)
		return sendMessageFromServ(fd, 403, vec[2] + " :No such channel");
	if (!channel->hasClient(fd))
		return sendMessageFromServ(fd, 442, vec[2] + " :You're not on that channel");
	if (channel->isInviteOnly() && !channel->isOperator(fd))
		return sendMessageFromServ(fd, 482, vec[2] + " :You're not channel operator");
	if (channel->hasClient(target->getFd()))
		return sendMessageFromServ(fd, 443, vec[1] + " " + vec[2] + " :Is already on channel");
	std::string msg = client->getPrefix() + " INVITE " + vec[1] + " " + vec[2];
	channel->addInvited(target->getFd());
	sendRawMessage(target->getFd(), msg + "\r\n");
	sendMessageFromServ(fd, 341, vec[1] + " " + vec[2]);
}

void	Server::whoCommand(int fd, std::vector<std::string> vec)
{
	Client*	requester = _clients[fd];
	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, "WHO :Not enough parameters");
	std::string target = vec[1];

	//channels
	if (target[0] == '#')
	{
		Channel* channel = getChannelByName(target);
		if (!channel)
			return sendMessageFromServ(fd, 403, target + " :No such channel");
		std::vector<Client*> client_list = channel->getClientList();
		for (size_t i = 0; i < client_list.size(); i++)
		{
			Client* client = client_list[i];
			std::string status = channel->isOperator(client->getFd()) ? "@" : "";
			std::string msg = ":" + _name + " 352 " + requester->getNickname() + " " + \
				target + " " + client->getUsername() + " " + client->getHostname() + " " + _name + \
				" " +  client->getNickname() + " H " + status + " :0 " + client->getRealname() + "\r\n";
			sendRawMessage(fd, msg);
		}
		std::string end_msg = ":" + _name + " 315 " + requester->getNickname() + " " + target + " :End of WHO list\r\n";
		return sendRawMessage(fd, end_msg);
	}

	//users
	Client*	user = findClientByNickname(target);
	if (user)
	{
		std::string status = "H";
		std::string channelName = "*";
		for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		{
			if (it->second->hasClient(user->getFd()))
			{
				channelName = it->first;
				if (it->second->isOperator(user->getFd()))
					status += "@";
				break;
			}
		}
		std::string reply = ":" + _name + " 352 " + requester->getNickname() + " " + \
			channelName + " " + user->getUsername() + " " + user->getHostname() + " " + \
			_name + " " + user->getNickname() + " " + status + " :0 " + user->getRealname() + "\r\n";
		sendRawMessage(fd, reply);
		std::string endMsg = ":" + _name + " 315 " + requester->getNickname() + " " + target + " :End of WHO list\r\n";
		return sendRawMessage(fd, endMsg);
	}
	//if nothing matches
	return sendMessageFromServ(fd, 403, target + " :No such nick/channel");
}

void	Server::namesCommand(int fd, std::vector<std::string> vec)
{
	Client* client = _clients[fd];
	if (vec.size() < 2 || vec[1].empty())
	{
		for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); it++)
		{
			Channel* channel = it->second;
			std::string channel_name = it->first;
			std::string names;
			std::vector<Client*> clients = channel->getClientList();
			for (size_t i = 0; i < clients.size(); i++)
			{
				if (channel->isOperator(clients[i]->getFd()))
					names += '@';
				names += clients[i]->getNickname();
				if (i + 1 < clients.size())
					names += " ";
			}
			std::cerr << names << std::endl;
			sendMessageFromServ(fd, 353, client->getNickname() + " = " + channel_name + " :" + names);
			sendMessageFromServ(fd, 366, client->getNickname() + " " + channel_name + " :End of NAMES list");
		}
		return ;
	}
	std::vector<std::string> names_list = splitList(vec[1]);
	for (size_t i = 0; i < names_list.size(); ++i)
	{
		std::string channel_name = names_list[i];
		Channel* channel = getChannelByName(channel_name);
		if (!channel)
			continue ;
		std::string names;
		std::vector<Client*> clients = channel->getClientList();
		for (size_t j = 0; j < clients.size(); ++j)
		{
			if (channel->isOperator(clients[j]->getFd()))
				names += "@";
			names += clients[j]->getNickname();
			if (j + 1 < clients.size())
				names += " ";
		}
		std::cerr << names << std::endl;
		sendMessageFromServ(fd, 353, client->getNickname() +  " = " + channel_name + " :" + names);
		sendMessageFromServ(fd, 366, client->getNickname() + " " + channel_name + " :End of NAMES list");
	}
}

void	Server::listCommand(int fd, std::vector<std::string> vec)
{
	Client* client = _clients[fd];
	std::string nick = client->getNickname();

	sendMessageFromServ(fd, 321, "Channel :Users Name"); // Start of list

	if (vec.size() < 2)
	{
		for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		{
			Channel* channel = it->second;
			std::string name = it->first;
			std::string topic = channel->getTopic();
			size_t userCount = channel->getClientList().size();
			std::stringstream oss;
			oss << name << " " << userCount << " :" << topic;
			sendMessageFromServ(fd, 322, oss.str());
		}
	}
	else
	{
		std::vector<std::string> list = splitList(vec[1]);
		for (size_t i = 0; i < list.size(); ++i)
		{
			std::string channel_name = list[i];
			Channel* channel = getChannelByName(channel_name);
			if (!channel)
				continue;
			std::string topic = channel->getTopic();
			size_t userCount = channel->getClientList().size();
			std::stringstream oss;
			oss << channel_name << " " << userCount << " :" << topic;
			sendMessageFromServ(fd, 322, oss.str());
		}
	}
	sendMessageFromServ(fd, 323, ":End of LIST");
}
