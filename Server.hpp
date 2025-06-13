/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 17:52:35 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/12 17:35:18 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
 #define SERVER_HPP

#include "includes.h"
#include "Client.hpp"

class Client;

class Server
{
private:
	int	_port;
	std::string	_password;
	int	_socket;
	std::vector<struct pollfd>	_poll_fds;
	std::map<int, Client*>	_clients;
public:
	Server(int port, std::string const &password);
	~Server();

	int	get_port() const;
	std::string const &	get_password() const;

	void	init_serv();
	void	setup_socket();
	void	accept_client();
	void	handle_client(int fd);
	void	ft_error_serv(std::string const & str);
};


#endif
