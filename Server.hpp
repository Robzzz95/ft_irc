/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 17:52:35 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/16 17:09:15 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
 #define SERVER_HPP

#include "includes.h"
#include "Client.hpp"

class Client;

class Server
{
private:
	int	_port;
	std::string	_password;
	int	_socket;
	std::vector<struct pollfd>	_poll_fds;
	std::map<int, Client*>	_clients;
	std::string	_name;
public:
	Server(int port, std::string const &password);
	~Server();

	int	get_port() const;
	std::string const &	get_password() const;

	void	initServ();
	void	setupSocket();
	void	acceptClient();
	void	handleClient(int fd);
	int	processCommand(int fd, const std::string & line);
	void	sendMessage(int fd, int code, const std::string &message);
	void	closeConnection(int fd);
	void	ftErrorServ(std::string const & str);

	void	passCommand(int fd, const std::string &line);
	void	nickCommand(int fd, const std::string &line);
	bool	isValidNickname(const std::string &nickname, const std::string &extra);
	void	userCommand(int fd, const std::string &line);
};


#endif
