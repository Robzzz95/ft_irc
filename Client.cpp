/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:33:05 by roarslan          #+#    #+#             */
/*   Updated: 2025/08/13 16:39:33 by roarslan         ###   ########.fr       */
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
	_buffer = "";
	_authentificated = false;
	_has_sent_nick = false;
	_registered = false;
	_prefix = "";
	_last_ping = -1;
	_last_activity = -1;
	_awaiting_pong = false;
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

std::string	Client::getPrefix() const
{
	return ":" + getNickname() + "!" + getUsername() + "@" + getHostname();
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

bool	Client::getHasSentNick() const
{
	return (_has_sent_nick);
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

void	Client::setHasSentNick(bool value)
{
	_has_sent_nick = value;
}

void	Client::setRegistered(bool value)
{
	_registered = value;
}

void	Client::appendToBuffer(const std::string &str)
{
	_buffer += str;
}

std::vector<std::string>	Client::extractLines()
{
	std::vector<std::string>	lines;
	size_t	pos;
	
	while ((pos = _buffer.find("\r\n")) != std::string::npos)
	{
		lines.push_back(_buffer.substr(0, pos));
		_buffer.erase(0, pos + 2);
	}
	return (lines);
}


bool	Client::isAwaitingPong() const
{
	return (_awaiting_pong);
}

void	Client::setAwaitingPong(bool value)
{
	_awaiting_pong = value;
}

void	Client::setLastPing(time_t time)
{
	_last_ping = time;
}

void	Client::setLastActivity(time_t time)
{
	_last_activity = time;
}

time_t	Client::getLastPing() const
{
	return (_last_ping);
}

time_t	Client::getLastActivity() const
{
	return (_last_activity);
}

void	Client::updateActivity()
{
	_last_activity = time(NULL);
	_awaiting_pong = false;
}
