/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 15:51:51 by roarslan          #+#    #+#             */
/*   Updated: 2025/08/03 17:15:56 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(const std::string &name)
{
	_name = name;
	 _topic = "";
	_is_invite_only = false;
	_is_topic_locked = false;
	_has_password = false;
	_has_limit = false;
	_limit = -1;
	_password = "";
}

Channel::~Channel()
{
}

const std::string&	Channel::getName() const
{
	return (_name);
}

const std::string&	Channel::getTopic() const
{
	return (_topic);
}

void	Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}

std::vector<Client*>	Channel::getClientList()
{
	std::vector<Client*>	list;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
		list.push_back(it->second);
	return (list);
}

bool	Channel::hasClient(int fd) const
{
	return (_clients.find(fd) != _clients.end());
}

void	Channel::addClient(int fd, Client* client)
{
	_clients[fd] = client;
	if (_clients.size() == 1)
		makeOperator(fd);
}

void	Channel::removeClient(int fd)
{
	_clients.erase(fd);
	_operators.erase(fd);
}

bool	Channel::isOperator(int fd) const
{
	return (_operators.find(fd) != _operators.end());
}

void	Channel::makeOperator(int fd)
{
	if (_clients.find(fd) != _clients.end())
		_operators.insert(fd);
}

void	Channel::removeOperator(int fd)
{
	_operators.erase(fd);
}

void	Channel::broadcast(const std::string &message, int except_fd)
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->first == except_fd)
			continue ;
		send(it->first, message.c_str(), message.size(), 0);
	}
}

bool	Channel::isEmpty() const
{
	return _clients.empty();
}

bool	Channel::isInviteOnly() const
{
	return (_is_invite_only);
}

void	Channel::setInviteOnly(bool value)
{
	_is_invite_only = value;
}

void	Channel::addInvited(int fd)
{
	_invited_clients.insert(fd);
}

void	Channel::removeInvited(int fd)
{
	_invited_clients.erase(fd);
}

bool	Channel::isInvited(int fd)
{
	return _invited_clients.count(fd) > 0;
}

bool	Channel::isTopicLocked() const
{
	return (_is_topic_locked);
}

void	Channel::setTopicLocked(bool value)
{
	_is_topic_locked = value;
}

bool	Channel::hasPassword() const
{
	return (_has_password);
}

void	Channel::setHasPassword(bool value)
{
	_has_password = value;
}

void	Channel::setPassword(const std::string &new_password)
{
	_password = new_password;
}

const std::string &	Channel::getPassword() const
{
	return (_password);
}

bool	Channel::hasLimit() const
{
	return (_has_limit);
}

void	Channel::setHasLimit(bool value)
{
	_has_limit = value;
}

int		Channel::getLimit() const
{
	return (_limit);	
}

void	Channel::setLimit(int new_limit)
{
	_limit = new_limit;
}
