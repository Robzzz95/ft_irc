/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 16:22:01 by roarslan          #+#    #+#             */
/*   Updated: 2025/08/11 17:17:05 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "includes.h"
#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"

bool	g_running = true;

void	signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		std::cout << RED << "\nSIGINT CAUGHT. SHUTTING DOWN..." << RESET << std::endl;
		g_running = false;
	}
}

int	main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << RED "Error: wrong arguments." RESET << std::endl;
		return (1);
	}

	std::signal(SIGINT, signalHandler);
	
	signal(SIGPIPE, SIG_IGN);// temp fix to handle broken pipe thingy
	
	arguments_parser(av);
	Server	serv(atoi(av[1]), av[2]);
	std::cout << YELLOW "STARTING SERVER..." RESET << std::endl;
	serv.initServ();
	return (0);
}
