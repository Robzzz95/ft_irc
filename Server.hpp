/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 17:52:35 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/24 14:14:59 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
 #define SERVER_HPP

#include "includes.h"
#include "Client.hpp"
#include "Channel.hpp"

class Client;

typedef	void	(Server::*commandHandler)(int, std::vector<std::string>);

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
	int		processCommand(int fd, const std::string & line);
	void	sendMessageFromServ(int fd, int code, const std::string &message);
	void	sendRawMessage(int fd, const std::string &message);
	void	closeConnection(int fd);
	void	ftErrorServ(std::string const & str);
	void	removeClientFromAllChannels(int fd);

	void	passCommand(int fd, std::vector<std::string> vec);
	void	nickCommand(int fd, std::vector<std::string> vec);
	bool	isValidNickname(const std::string &nickname);
	void	userCommand(int fd, std::vector<std::string> vec);
	void	quitCommand(int fd, std::vector<std::string> vec);
	void	privmsgCommand(int fd, std::vector<std::string> vec);
	Client*		findClientByNickname(const std::string &nickname);
	Channel*	getChannelByName(const std::string &str);
	void	pingCommand(int fd, std::vector<std::string> vec);
	void	joinCommand(int fd, std::vector<std::string> vec);
	std::vector<std::string>	splitChannels(const std::string &str);
	void	capCommand(int fd, std::vector<std::string> vec);
	void	modeCommand(int fd, std::vector<std::string> vec);
	void	whoisCommand(int fd, std::vector<std::string> vec);
};


#endif
