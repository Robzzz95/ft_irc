/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 16:22:01 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/29 16:42:19 by roarslan         ###   ########.fr       */
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
	
	arguments_parser(av);
	Server	serv(atoi(av[1]), av[2]);
	std::cout << YELLOW "STARTING SERVER..." RESET << std::endl;
	serv.initServ();
	return (0);
}


/*
to do list:
get pingpong command to count time and respond better
operators handling with all the MODE commands
get the modes for channels
get ctrl+Z to work properly
some inputs outputs should get done properly, might have issues so far 
add missing commands
join and invite add limit to channels
join should check for restricted channels and check for _invited_clients
part should delete a channel while the bot is still in it ?
*/