/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/15 16:56:22 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(int port, std::string const &password) : _port(port), _password(password), _socket(-1)
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

	////////////////////////////////// fonction interdite?
	// std::string hostname;
	// struct hostent* host = gethostbyaddr(&client_addr.sin_addr, sizeof(client_addr.sin_addr), AF_INET);
	// if (host && host->h_name)
	// 	hostname = host->h_name;
	// else
	// 	hostname = ip;
	// //////////////////////////////////

	_clients[client_fd] = new Client(client_fd, ip, hostname);
	std::cout << "Accepted new client on FD: " << client_fd << std::endl;
	std::cout << "	New clients ip: " << ip << ", its name is: " << hostname << std::endl;
	sendMessage(client_fd, "Please enter password using: [PASS <password>]\n");
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

void	Server::processCommand(int fd, const std::string &line)
{
	Client*	client = _clients[fd];
	std::istringstream iss(line);
	std::string	command;
	iss >> command;
	
	if (command == "PASS" && !client->getAuthentificated())
	{
		std::string pass;
		iss >> pass;
		if (pass != _password)
		{
			sendMessage(fd, "Error: wrong password.\nYou have been disconnected.\r\n");
			closeConnection(fd);
			return ;
		}
		client->setAuthentificated(true);
		sendMessage(fd, "Correct password.\r\n");
	}
	else if (command == "PASS" && client->getAuthentificated())
		sendMessage(fd, "Already authentificated.\n");
	// else if (client->getAuthentificated())
	// 		std::cout << line << std::endl;
}

void	Server::sendMessage(int fd, const std::string &message)
{
	ssize_t sent = send(fd, message.c_str(), message.length(), 0);
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