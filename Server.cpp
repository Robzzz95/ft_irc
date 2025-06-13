/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:29:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/13 18:31:39 by roarslan         ###   ########.fr       */
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

void	Server::ft_error_serv(std::string const & str)
{
	if (_socket != -1)
		close(_socket);
	std::cerr << RED "Error: " RESET << str << std::endl;
	exit(1);
}

void	Server::init_serv()
{
	setup_socket();
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
					accept_client();
				else
					handle_client(_poll_fds[i].fd);
			}
		}
	}
}

void	Server::setup_socket()
{
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		ft_error_serv("socket() failed.");
	int opt = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		ft_error_serv("setsockopt() failed.");
	if (fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
		ft_error_serv("fcntl() failed.");

	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family  =AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);

	if (bind(_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		ft_error_serv("bind() failed.");
	if (listen(_socket, SOMAXCONN) < 0)
		ft_error_serv("listen() failed.");
	
	struct pollfd	pfd;
	pfd.fd = _socket;
	pfd.events = POLLIN;
	_poll_fds.push_back(pfd);
	std::cout << GREEN << "SERVER LISTENING ON PORT " << RESET << _port << std::endl;
}

void	Server::accept_client()
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int	client_fd = accept(_socket, (struct sockaddr*)&client_addr, &addr_len);
	
	if (client_fd < 0)
	{
		std::cerr << "Error accepting client." << std::endl;
		return ;
	}
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	
	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	_poll_fds.push_back(pfd);
	////////////////////////////////// fonction interdite?
	struct hostent* host = gethostbyaddr(&client_addr.sin_addr, sizeof(client_addr.sin_addr), AF_INET);
	char*	hostname = host->h_name;
	if (!hostname)
		hostname = inet_ntoa(client_addr.sin_addr);
	//////////////////////////////////
	_clients[client_fd] = new Client(client_fd, inet_ntoa(client_addr.sin_addr), hostname);

	std::cout << "Accepted new client on FD: " << client_fd << std::endl;

	std::cout << "	New clients ip: " << inet_ntoa(client_addr.sin_addr) << ", its name is: " << hostname << std::endl;
}

void	Server::handle_client(int fd)
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

	// Client *client = _clients[fd];
	// client->append_to_buffer(std::string(buffer, bytes_read));
	// client->process_buffer(*this);
}
