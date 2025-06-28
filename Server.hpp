/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 17:52:35 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/28 14:21:15 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
 #define SERVER_HPP

#include "includes.h"
#include "Client.hpp"
#include "Channel.hpp"

class Client;

typedef	void	(Server::*commandHandler)(int, const std::string &);

class Server
{
private:
	int	_port;
	std::string	_password;
	int	_socket;
	std::vector<struct pollfd>	_poll_fds;
	std::map<int, Client*>	_clients;
	std::map<std::string, Channel*>	_channels;
	std::string	_name;
	std::string _info;
	std::map<std::string, commandHandler>	_commands;
public:
	Server(int port, std::string const &password);
	~Server();

	int	get_port() const;
	std::string const &	get_password() const;

	void	initServ();
	void	setupSocket();
	void	initCommands();
	void	acceptClient();
	void	handleClient(int fd);
	int	processCommand(int fd, const std::string & line);
	void	sendMessageFromServ(int fd, int code, const std::string &message);
	void	sendRawMessage(int fd, const std::string &message);
	void	closeConnection(int fd);
	void	ftErrorServ(std::string const & str);

	void	passCommand(int fd, const std::string &line);
	void	nickCommand(int fd, const std::string &line);
	bool	isValidNickname(const std::string &nickname, const std::string &extra);
	void	userCommand(int fd, const std::string &line);
	void	quitCommand(int fd, const std::string &line);
	void	privmsgCommand(int fd, const std::string &line);
	Client*	findClientByNickname(const std::string &nickname);
	void	pingCommand(int fd, const std::string &line);
	void	joinCommand(int fd, const std::string &line);
	std::vector<std::string>	splitChannels(const std::string &str);
	void	capCommand(int fd, const std::string &line);
	void	modeCommand(int fd, const std::string &line);
	void	whoisCommand(int fd, const std::string &line);
};


#endif
