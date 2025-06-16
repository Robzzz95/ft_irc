/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/16 17:26:51 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _socket(-1), _name("ft_irc")
{
}

Server::~Server()
{
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

void	Server::initServ()
{
	setupSocket();
	while (42)
	{
		int ret = poll(&_poll_fds[0], _poll_fds.size(), -1);
		if (ret < 0)
		{
			std::cerr << "poll() failed." << std::endl;
			break ;
		}
		for (size_t i = 0; i < _poll_fds.size(); i++)
		{
			if (_poll_fds[i].revents & POLLIN)
			{
				if (_poll_fds[i].fd == _socket)
					acceptClient();
				else
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
	_poll_fds.push_back(pfd);

	std::string ip = inet_ntoa(client_addr.sin_addr);
	std::string hostname = ip;
	_clients[client_fd] = new Client(client_fd, ip, hostname);
	std::cout << "Accepted new client on FD: " << client_fd << std::endl;
	std::cout << "	New clients ip: " << ip << ", its hostname is: " << hostname << std::endl;
	sendMessage(client_fd, 0, "Please enter password using: PASS <password>");
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
	Client *client = _clients[fd];
	client->appendToBuffer(std::string(buffer, bytes_read));
	std::vector<std::string> lines = client->extractLines();
	for (size_t i = 0; i < lines.size(); i++)
		processCommand(fd, lines[i]);
}

int	Server::processCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string	command;
	iss >> command;

	if (command == "PASS")
		return (passCommand(fd, line), 0);
	else if (command == "NICK")
		return (nickCommand(fd, line), 0);
	else if (command == "USER")
		return (userCommand(fd, line), 0);

	if (!client->getAuthentificated())
		return (sendMessage(fd, 451, "Error: you must authentificate first."), 0);
	if (!client->getRegistered() && client->getAuthentificated())
		return (sendMessage(fd, 451, "You must register first."), 0);
	else
		std::cout << line << std::endl;
	return (0);
}

void	Server::sendMessage(int fd, int code, const std::string &message)
{
	Client*	client = _clients[fd];
	std::ostringstream oss;
	if (code != 0)
	{
		oss << ":" << _name << " " \
			<< std::setfill('0') << std::setw(3) << code << " " \
			<< (client->getNickname().empty() ? "*" : client->getNickname()) << " :" \
			<< message << "\r\n";
	}
	else
	{
		oss << ":" << _name << " " \
			<< (client->getNickname().empty() ? "*" : client->getNickname()) << " :" \
			<< message << "\r\n";
	}
	std::string str = oss.str();
	ssize_t sent = send(fd, str.c_str(), str.length(), 0);
	if (sent < 0)
	std::cerr << "Error : failed to send message to FD: " << fd << std::endl;
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
		sendMessage(fd, 462, "Already authentificated.");
		return ;
	}
	
	std::istringstream iss(line);
	std::string command, pass, extra;
	iss >> command >> pass >> extra;
	if (!extra.empty())
	{
		sendMessage(fd, 464, "Error: PASS command takes only one argument.");
		return ;
	}
	if (pass != _password)
	{
		sendMessage(fd, 464, "Error: wrong password.\nYou have been disconnected.");
		closeConnection(fd);
		return ;
	}
	client->setAuthentificated(true);
	sendMessage(fd, 0, "Correct password.\nPlease set your nickname using: NICK <nickname>");
}

void	Server::nickCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	
	if (!client->getAuthentificated())
	{
		sendMessage(fd, 451, "Error: you must authentificate first.");
		return ;		
	}
	std::istringstream iss(line);
	std::string command, nickname, extra_test;
	iss >> command >> nickname >> extra_test;
	if (!isValidNickname(nickname, extra_test))
	{
		sendMessage(fd, 432, "Error: invalid nickname.");
		return ;
	}
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->getNickname() == nickname)
		{
			sendMessage(fd, 433, "Error 433: nickname already in use.");
			return ;
		}
	}
	client->setNickname(nickname);
	sendMessage(fd, 0, ("Nickname set to " + nickname + \
		".\nPlease set your username and your real name using: USER <username> <0> <*> <realname>"));
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
		sendMessage(fd, 462, "Error: You are already registered.");
		return ;
	}
	if (!client->getAuthentificated())
	{
		sendMessage(fd, 451, "Error: you must authentificate first.");
		return ;
	}
	if (client->getNickname().empty())
	{
		sendMessage(fd, 431, "Error: Please set your nickname first.");
		return ;
	}
	std::istringstream iss(line);
	std::string command, username, ignore1, ignore2, realname;
	iss >> command >> username >> ignore1 >> ignore2;
	std::getline(iss, realname);
	if (username.empty() || realname.empty())
	{
		sendMessage(fd, 0, "Error: invalid USER command format.");
		return ;
	}
	size_t i = 0;
	while (realname[i] == ':' || realname[i] == ' ')
		i++;
	realname.erase(0, i);
	client->setUsername(username);
	client->setRealname(realname);
	client->setRegistered(true);
	sendMessage(fd, 001, client->getNickname() + " :Welcome to the IRC Network " +
		client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname());
	sendMessage(fd, 001, client->getNickname() + " :Your host is ft_irc, running version v1.0");
	sendMessage(fd, 001, client->getNickname() + " :This server was created Mon Jun 10 2025 at 13:45:00");
	sendMessage(fd, 001, client->getNickname() + " ft_irc v1.0 ao mtov");
}
