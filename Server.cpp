/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/28 14:27:35 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _socket(-1), _name("ft_irc"), _info("My first irc server")
{
}

Server::~Server()
{
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
	std::cout << YELLOW << "\nSERVER CLEANUP...\n" << GREEN << "\nSERVER CLEANUP COMPLETE" << RESET << std::endl;
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
	_commands["JOIN"] = &Server::joinCommand;
	_commands["CAP"] = &Server::capCommand;
	_commands["MODE"] = &Server::modeCommand;
	// _commands["WHOIS"] = &Server::whoisCommand;
	// _commands[] = &Server:: ;
	// _commands[] = &Server:: ;
	// _commands[] = &Server:: ;
	// _commands[] = &Server:: ;
	// _commands[] = &Server:: ;

}

void	Server::initServ()
{
	setupSocket();
	initCommands();
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
	addr.sin_family  =AF_INET;
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
	// sendMessageFromServ(client_fd, 0, "Please enter password using: PASS <password>");
	sendRawMessage(client_fd, ":ft_irc NOTICE * : Please enter your password using <PASS>\r\n");
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
	client->appendToBuffer(std::string(buffer, bytes_read));
	std::vector<std::string> lines = client->extractLines();
	for (size_t i = 0; i < lines.size(); i++)
		processCommand(fd, lines[i]);
}


//refaire une map avec les pointeurs de fonctions 
int	Server::processCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string	command;
	iss >> command;

	std::map<std::string, commandHandler>::iterator it = _commands.find(command);
	if (it != _commands.end())
	{
		commandHandler	handler = it->second;
		(this->*handler)(fd, line);
		return (0);
	}

	// if (command == "PASS")
	// 	return (passCommand(fd, line), 0);
	// else if (command == "NICK")
	// 	return (nickCommand(fd, line), 0);
	// else if (command == "USER")
	// 	return (userCommand(fd, line), 0);
	// else if (command == "PRIVMSG")
	// 	return (privmsgCommand(fd, line), 0);
	// else if (command == "QUIT" || command == "EXIT")
	// 	return (quitCommand(fd, line), 0);
	// else if (command == "PING")
	// 	return (pingCommand(fd, line), 0);
	// else if (command == "JOIN")
	// 	return (joinCommand(fd, line), 0);
	// else if (command == "CAP")
	// 	return (capCommand(fd, line), 0);
	// else if (command == "MODE")
	// 	return (modeCommand(fd, line), 0);
	// else if (command == "PING")
	// 	return (pingCommand(fd, line), 0);
	// else if (command == "WHOIS")
	// 	return(whoisCommand(fd, line), 0);

	if (!client->getAuthentificated())
		return (sendMessageFromServ(fd, 451, "Error: you must authentificate first."), 1);
	if (!client->getRegistered() && client->getAuthentificated())
		return (sendMessageFromServ(fd, 451, "You must register first."), 1);
	else
		std::cout << line << std::endl;
	return (0);
}

void	Server::sendMessageFromServ(int fd, int code, const std::string &message)
{
	Client*	client = _clients[fd];
	std::ostringstream oss;
	if (code != 0)
	{
		oss << ":" << _name << " " \
			<< std::setfill('0') << std::setw(3) << code << " " \
			<< (client->getNickname().empty() ? "*" : client->getNickname()) << ": " \
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
		delete _clients[fd];
		_clients.erase(fd);
	}
	std::cout << "Closed connection on FD: " << fd << std::endl;
}

void	Server::passCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	if (client->getAuthentificated())
	{
		sendMessageFromServ(fd, 462, "Already authentificated.");
		return ;
	}
	
	std::istringstream iss(line);
	std::string command, pass, extra;
	iss >> command >> pass >> extra;
	if (!extra.empty())
	{
		sendMessageFromServ(fd, 464, "Error: PASS command takes only one argument.");
		return ;
	}
	if (pass != _password)
	{
		sendMessageFromServ(fd, 464, "Error: wrong password.\nYou have been disconnected.");
		closeConnection(fd);
		return ;
	}
	client->setAuthentificated(true);
	// sendMessageFromServ(fd, 0, "Correct password.\nPlease set your nickname using: NICK <nickname>");
}

//notify everyone in the channel of change of nickname
void	Server::nickCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	
	// if (!client->getAuthentificated())
	// {
	// 	sendMessageFromServ(fd, 451, "Error: you must authentificate first.");
	// 	return ;		
	// }
	std::istringstream iss(line);
	std::string command, nickname, extra_test;
	iss >> command >> nickname >> extra_test;
	if (!isValidNickname(nickname, extra_test))
	{
		sendMessageFromServ(fd, 432, "Error: invalid nickname.");
		client->setAuthentificated(true);
		return ;
	}
	if (!client->getAuthentificated())
	{
		client->setNickname(nickname);
		return ;
	}
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->getNickname() == nickname)
		{
			sendMessageFromServ(fd, 433, "Error 433: nickname already in use.");
			return ;
		}
	}
	// std::string oldnick = client->getNickname();
	// client->setNickname(nickname);
	// if (oldnick.empty())
	// {
	// 	sendMessageFromServ(fd, 001, ("Nickname set to " + nickname + \
	// 		".\nPlease set your username and your real name using: USER <username> <0> <*> <realname>"));
	// 	client->setNickname(nickname);
	// 	return ;
	// }

	sendRawMessage(fd, (":" + client->getNickname() + "!" + client->getUsername() \
		+ "@" + client->getRealname() + " NICK " + nickname + "\r\n"));
	client->setNickname(nickname);
	// sendMessageFromServ(fd, 451, "you must register first.");
}

bool	Server::isValidNickname(const std::string &nickname, const std::string &extra)
{
	if (nickname.empty() || nickname.size() > 9 || !extra.empty())
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

void	Server::userCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	if (client->getRegistered())
	{
		sendMessageFromServ(fd, 462, "Error: You are already registered.");
		return ;
	}
	// if (!client->getAuthentificated())
	// {
	// 	sendMessageFromServ(fd, 451, "Error: you must authentificate first.");
	// 	return ;
	// }
	// if (client->getNickname().empty())
	// {
	// 	sendMessageFromServ(fd, 431, "Error: Please set your nickname first.");
	// 	return ;
	// }
	std::istringstream iss(line);
	std::string command, username, ignore1, ignore2, realname;
	iss >> command >> username >> ignore1 >> ignore2;
	std::getline(iss, realname);
	if (username.empty() || realname.empty())
	{
		sendMessageFromServ(fd, 0, "Error: invalid USER command format.");
		return ;
	}
	size_t i = 0;
	while (realname[i] == ':' || realname[i] == ' ')
		i++;
	realname.erase(0, i);
	client->setUsername(username);
	client->setRealname(realname);
	client->setRegistered(true);
	sendMessageFromServ(fd, 001, client->getNickname() + " :Welcome to the IRC Network " +
		client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname());
	sendMessageFromServ(fd, 002, client->getNickname() + " :Your host is ft_irc, running version v1.0");
	sendMessageFromServ(fd, 003, client->getNickname() + " :This server was created Mon Jun 10 2025 at 13:45:00");
	sendMessageFromServ(fd, 004, client->getNickname() + " ft_irc v1.0 ao mtov");
}

void	Server::quitCommand(int fd, const std::string &line)
{
	// Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string command, reason, message;
	iss >> command;
	std::getline(iss, reason);
	if (!reason.empty())
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

void	Server::privmsgCommand(int fd, const std::string &line)
{
	Client*	sender = _clients[fd];
	if (!sender->getRegistered())
	{
		sendMessageFromServ(fd, 451, "You have not registered.");
		return ;
	}
	std::istringstream iss(line);
	std::string command, recipient, message;
	iss >> command >> recipient;
	std::getline(iss, message);
	if (!message.empty() && message[0] == ':')
		message.erase(0, 1);
	if (recipient.empty() || message.empty())
	{
		sendMessageFromServ(fd, 461, "PRIVMSG: not enough parameters.");
		return ;
	}
	Client*	target = findClientByNickname(recipient);
	if (!target)
	{
		sendMessageFromServ(fd, 401, recipient + " : no such nick.");
		return ;
	}
	std::string full_message = ":" + sender->getNickname() + " PRIVMSG " + " :" + message + "\r\n";
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

void	Server::pingCommand(int fd, const std::string &line)
{
	std::istringstream iss(line);
	std::string cmd, token;
	iss >> cmd >> token;
	if (token.empty())
	{
		sendRawMessage(fd, ":PONG " + _name + "\r\n");
		return ;
	}
	sendRawMessage(fd, ":PONG " + token + "\r\n");
}

void	Server::joinCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	if (!client->getRegistered())
	{
		sendMessageFromServ(fd, 451, "you must register first.");
		return ;
	}
	
	std::istringstream	iss(line);
	std::string cmd, channel_list;
	iss >> cmd >> channel_list;
	std::vector<std::string>	channels = splitChannels(channel_list);
	for (size_t i = 0; i < channels.size(); i++)
	{
		std::string &channel_name = channels[i];	
		if (channel_name.empty() || channel_name[0] != '#')
		{
			sendMessageFromServ(fd, 476, "Invalid channel name.");
			return ;
		}
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
		_channels[channel_name]->broadcast(message);
		sendRawMessage(fd, ":ft_irc 331 " + client->getNickname() + " " + channel_name + " :no topic is set.\r\n");
	}
}

std::vector<std::string>	Server::splitChannels(const std::string &str)
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

void	Server::capCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string command, param;
	iss >> command >> param;
	if ((!client->getAuthentificated() && !client->getRegistered()) && param != "END")
	{
		std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
		sendRawMessage(fd, ":irc.yourserver.net CAP " + nick + " LS :\r\n");
	}
	else if (param == "END")
		return ;
}

void	Server::modeCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string command, name, mode;
	iss >> command >> name >> mode;
	// if (!name || !mode)
	// {
	// 	//gerer l'erreur
	// }
	std::string msg = client->getPrefix() + " MODE " + client->getNickname() + " " + mode;
	sendRawMessage(fd, msg);
}

//ajouter la gestion de channels
void	Server::whoisCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string command, name;
	iss >> command >> name;

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
