/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 15:51:51 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/24 16:55:36 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(const std::string &name) : _name(name), _topic("")
{
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
