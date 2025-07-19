/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 15:39:00 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/19 12:04:43 by roarslan         ###   ########.fr       */
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

	// bool	_invite_only;
	// std::string	_password;
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
	bool	hasPassword() const;
	bool	isTopicLocked()	const;
	void	setMode(char mode);

	void	broadcast(const std::string &message);


};



#endif