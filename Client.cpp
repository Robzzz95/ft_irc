/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:33:05 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/13 18:20:11 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int fd, const std::string &ip, const std::string &hostname)
{
	_fd = fd;
	_ip = ip;
	_hostname = hostname;
	_nickname = "";
	_username = "";
	_realname = "";
	_authentificated = false;
	_registered = false;
	// _disconnected = false;
}

Client::~Client()
{
}


int Client::getFd() const
{
	return (_fd);
}

const std::string &	Client::getHostname() const
{
	return (_hostname);
}

const std::string &	Client::getNickname() const
{
	return (_nickname);
}

const std::string &	Client::getUsername() const
{
	return (_username);
}

const std::string &	Client::getRealname() const
{
	return (_realname);
}

const std::string &	Client::getBuffer() const
{
	return (_buffer);	
}

std::string & Client::getBufferMutable()
{
	return (_buffer);
}

bool	Client::getAuthentificated() const
{
	return (_authentificated);
}

bool	Client::getRegistered() const
{
	return (_registered);
}

void	Client::setNickname(const std::string &str)
{
	_nickname = str;
}

void	Client::setUsername(const std::string &str)
{
	_username = str;
}

void	Client::setRealname(const std::string &str)
{
	_realname = str;
}

void	Client::setAuthentificated(bool value)
{
	_authentificated = value;
}

void	Client::setRegistered(bool value)
{
	_registered = value;
}
