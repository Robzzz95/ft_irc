/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 16:22:01 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/12 14:25:50 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "includes.h"
#include "Client.hpp"
#include "Server.hpp"

int	main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << RED "Error: wrong arguments." RESET << std::endl;
		return (1);
	}
	
	arguments_parser(av);
	Server	serv(atoi(av[1]), av[2]);
	// std::cout << "Port: " << serv.get_port() << "\nPassword: " << serv.get_password() << std::endl;
	std::cout << YELLOW "STARTING SERVER..." RESET << std::endl;
	serv.init_serv();
	return (0);
}
