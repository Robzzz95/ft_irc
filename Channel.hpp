/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 15:39:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/29 12:42:32 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
 #define CHANNEL_HPP

#include "includes.h"

class Client;

class Channel
{
private:
	std::string _name;
	std::string _topic;
	std::map<int, Client*>	_clients;
	std::set<int>	_operators;

	bool	_is_invite_only;
	bool	_is_topic_locked;
	bool	_has_password;
	bool	_has_limit;
	int		_limit;
	std::string	_password;
	std::set<int> _invited_clients;
public:
	Channel(const std::string &name);
	~Channel();

	const std::string&	getName() const;
	std::vector<Client*>	getClientList();
	const std::string& getTopic() const;
	void	setTopic(const std::string &topic);

	bool	hasClient(int fd) const;
	void	addClient(int fd, Client* client);
	void	removeClient(int fd);
	
	bool	isOperator(int fd) const;
	void	makeOperator(int fd);
	void	removeOperator(int fd);

	bool	isInviteOnly() const;
	void	setInviteOnly(bool value);
	void	addInvited(int fd);
	void	removeInvited(int fd);
	bool	isInvited(int fd);
	
	bool	isTopicLocked()	const;
	void	setTopicLocked(bool value);

	bool	hasPassword() const;
	void	setHasPassword(bool value);
	void	setPassword(const std::string &new_password);
	const std::string &	getPassword() const;

	bool	hasLimit() const;
	void	setHasLimit(bool value);
	int		getLimit() const;
	void	setLimit(int new_limit);

	bool	isEmpty() const;

	void	broadcast(const std::string &message, int except_fd);
};



#endif