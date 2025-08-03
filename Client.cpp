/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 17:33:05 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/31 16:13:28 by sacha            ###   ########.fr       */
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
	_registered = false;
	_prefix = "";
	_last_activity = time(NULL);
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

void Client::updateActivity()
{
	_last_activity = time(NULL);
}

bool Client::isInactive(time_t timeout) const
{
	return (time(NULL) - _last_activity) > timeout;
}

void Client::queueMessage(const std::string &message)
{
	_pending_messages.push(message);
}

std::vector<std::string> Client::getPendingMessages()
{
	std::vector<std::string> messages;
	while (!_pending_messages.empty())
	{
		messages.push_back(_pending_messages.front());
		_pending_messages.pop();
	}
	return messages;
}

