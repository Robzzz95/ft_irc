/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 17:53:32 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/31 16:13:28 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
 #define CLIENT_HPP
 
#include "includes.h"

class Server;

class Client
{
private:
	int _fd;
	std::string	_ip;
	std::string	_hostname;
	std::string	_nickname;
	std::string	_username;
	std::string	_realname;

	std::string	_buffer;
	std::queue<std::string> _pending_messages;
	time_t _last_activity;
	
	bool	_authentificated;
	bool	_registered;
	
	std::string _prefix;
	
public:
	Client(int fd, const std::string &ip, const std::string &hostname);
	~Client();

	int getFd() const;
	const std::string &	getHostname() const;
	const std::string &	getNickname() const;
	const std::string &	getUsername() const;
	const std::string &	getRealname() const;
	const std::string &	getBuffer() const;
	std::string	getPrefix() const;
	std::string & getBufferMutable();
	bool	getAuthentificated() const;
	bool	getRegistered() const;

	void	setNickname(const std::string &str);
	void	setUsername(const std::string &str);
	void	setRealname(const std::string &str);
	void	setAuthentificated(bool value);
	void	setRegistered(bool value);

	void	appendToBuffer(const std::string &str);
	std::vector<std::string>	extractLines();
	
	void updateActivity();
	bool isInactive(time_t timeout) const;
	void queueMessage(const std::string &message);
	std::vector<std::string> getPendingMessages();
};



#endif