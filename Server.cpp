/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/29 16:53:13 by roarslan         ###   ########.fr       */
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
	_commands["MODE"] = &Server::modeCommand;
	_commands["WHOIS"] = &Server::whoisCommand;
	_commands["PART"] = &Server::partCommand;
	_commands["KICK"] = &Server::kickCommand;
	_commands["INVITE"] = &Server::inviteCommand;
	_commands["TOPIC"] = &Server::topicCommand;
	// _commands["WHO"] = &Server:: ;
	// _commands["NOTICE"] = &Server:: ;
	// _commands["NAMES"] = &Server:: ;
	// _commands["LIST"] = &Server:: ;
	// _commands[] = &Server:: ; //send file
	// _commands[] = &Server:: ; //receive file
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
	std::cout << GREEN << "SERVER LISTENING ON PORT " << RESET << _port << std::endl;
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
		if (bytes_read == 0 || (bytes_read < 0 && errno != EWOULDBLOCK && errno != EAGAIN))
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
		
		//rajouter une meilleure gestion de commandes non existantes
		if (_clients.find(fd) == _clients.end())
			return (1);
	}
	return (0);
}

void	Server::sendMessageFromServ(int fd, int code, const std::string &message)
{
	Client*	client = _clients[fd];
	std::ostringstream oss;
	if (code != 0)
	{
		oss << ":" << _name << " " \
			<< std::setw(3) << std::setfill('0') << code << " " \
			<< (client->getNickname().empty() ? "*" : client->getNickname()) << " :" \
			<< message << "\r\n";
	}
	else
	{
		oss << ":" << _name << " " \
			<< (client->getNickname().empty() ? "*" : client->getNickname()) << ": " \
			<< message << "\r\n";
	}
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
			
			//!!broadcast about leaving to other clients in channel!!!
			
			//std::string partingMsg = : + _clients[fd]->getPrefix + " PART " + channel->getName() + \r\n;
			//channel->broadcast(partingMsg);
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

//notify everyone in the channel of change of nickname
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
		return sendMessageFromServ(fd, 464, "Error: NICK wrong parameters");
	if (!isValidNickname(vec[1]))
		return sendMessageFromServ(fd, 432, "Error: invalid nickname.");
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first != fd && it->second->getNickname() == vec[1])
			return sendMessageFromServ(fd, 433, "Nickname already in use."), closeConnection(fd);
	}
	sendRawMessage(fd, (client->getPrefix() + " NICK :" + vec[1] + "\r\n"));
	client->setNickname(vec[1]);
	//notifier tous les channels
}

bool	isValidNickname(const std::string &nickname)
{
	if (nickname.empty() || nickname.size() > 9)
		return (false);

	char c = nickname[0];
	if (!isalpha(c) && std::string (" ,*?!@.:#[]\\`^{}").find(c) == std::string::npos)
		return (false);
	for (size_t i = 1; i < nickname.size(); i++)
	{
		c = nickname[i];
		if (!isalnum(c) && std::string(" ,*?!@.:#[]\\`^{}").find(c) == std::string::npos)
			return (false);
	}
	return (true);
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
		sendMessageFromServ(fd, 0, "Error: invalid USER command format.");//a corriger
		return ;
	}
	std::string username = vec[1];
	std::string realname = vec[4]; 
	if (username.empty() || realname.empty())
	{
		sendMessageFromServ(fd, 0, "Error: invalid USER command format.");//a corriger
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
	sendMessageFromServ(fd, 002, client->getNickname() + " :Your host is " + _name + " , running version v1.0");
	sendMessageFromServ(fd, 003, client->getNickname() + " :This server was created Mon Jun 10 2025 at 13:45:00");
	sendMessageFromServ(fd, 004, _name + " v1.0 o o");
	// sendMessageFromServ(fd, 005, client->getNickname() + "CHANTYPES=# PREFIX=(o)@ CHANMODES=o");
	// sendMessageFromServ(fd, 376, "End of /MOTD command.");
}

void	Server::quitCommand(int fd, std::vector<std::string> vec)
{
	std::string reason = vec[1];
	std::string message; /// a envoyer a tout le monde pour avertir du depart!
	if (vec.size() > 2)
	{
		size_t i = 0;
		while (i < reason.size() && (reason[i] == ':' || reason[i] == ' '))
			i++;
		reason = reason.substr(i);
		if (!reason.empty())
			message = reason;
	}
	sendMessageFromServ(fd, 0, "closing connection\nthank you for using our server!\r\n");
	//////////////////////////////////////////notifier tout le monde sur channel!!!!!!!!!!!!!!!
	closeConnection(fd);
}

void	Server::privmsgCommand(int fd, std::vector<std::string> vec)
{
	Client*	sender = _clients[fd];
	if (!sender->getRegistered())
		return sendMessageFromServ(fd, 451, "You have not registered.");
	if (vec.size() < 2)
		return sendMessageFromServ(fd, 461, "PRIVMSG: not enough parameters.");
	
	std::string command, recipient, message;
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
		return sendMessageFromServ(fd, 401, recipient + " : no such nick.");
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
	Client* client = _clients[fd];
	client->updateActivity();
}

void	Server::pingCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	std::string token = (vec.size() >= 2) ? vec[1] : _name;
	client->setLastActivity(time(NULL));
	sendRawMessage(fd, ":" + _name + " PONG :" + token + "\r\n");
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

void	Server::joinCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (!client->getRegistered())
		return sendMessageFromServ(fd, 451, "you must register first.");
	if (vec.size() < 2 || vec[1].empty())
		return sendMessageFromServ(fd, 461, "JOIN: not enough parameters.");
	std::vector<std::string>	channels = splitList(vec[1]);

	for (size_t i = 0; i < channels.size(); i++)
	{
		std::string &channel_name = channels[i];	
		if (channel_name.empty() || channel_name[0] != '#')
			return sendMessageFromServ(fd, 476, "Invalid channel name.");
		if (_channels.find(channel_name) == _channels.end())
		{
			Channel*	new_channel = new Channel(channel_name);
			new_channel->addClient(fd, client);
			_channels[channel_name] = new_channel;
		}
		else
			_channels[channel_name]->addClient(fd, client);
		std::string message = ":" + client->getNickname() + "!" + client->getUsername() \
		+ "@" + client->getHostname() + " JOIN " + channel_name + "\r\n";
		_channels[channel_name]->broadcast(message, 0);
		sendRawMessage(fd, ":ft_irc 331 " + client->getNickname() + " " + channel_name + " :no topic is set.\r\n");
	}
}


//ajouter la raison du depart
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

std::vector<std::string>	Server::splitList(const std::string &str)
{
	std::vector<std::string>	dest;
	std::string	tmp;

	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == ',')
		{
			if (!tmp.empty())
				dest.push_back(tmp);
			tmp.clear();
		}
		else
			tmp += str[i];
	}
	if (!tmp.empty())
		dest.push_back(tmp);	
	return (dest);
}

void	Server::capCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 2)
	{
		//erreur pas assez d'arguments
		return ;
	}
	std::string param = vec[1];
	if ((!client->getAuthentificated() && !client->getRegistered()) && param != "END")
	{
		std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
		sendRawMessage(fd, ":irc.yourserver.net CAP " + nick + " LS :\r\n");
	}
	else if (param == "END")
		return ;
}

void	Server::modeCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
	if (vec.size() < 3)
	{
		//gerer l'erreur pas assez de parametres
		return ;
	}
	std::string name = vec[1];
	std::string mode = vec[2];
	// Channel* channel = getChannelByName(vec[1]);
	// if (!channel)
	// {
	// 	//erreur
	// 	return ;
	// }
	// if (mode == "-o")
	// {
	// 	// channel->setPrivileges(vec[3]);
	// 	return ;
	// }
	// else if (mode == "-i")
	// {
		
	// }
	// if (!name || !mode)
	// {
	// 	//gerer l'erreur
	// }
	std::string msg = client->getPrefix() + " MODE " + client->getNickname() + " " + mode;
	sendRawMessage(fd, msg);
}

//ajouter la gestion de channels
void	Server::whoisCommand(int fd, std::vector<std::string> vec)
{
	Client*	client = _clients[fd];
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
	// sendRawMessage(fd, msg);
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
		return sendMessageFromServ(fd, 461, "INVITE : Not enough parameters");
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
