/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 14:25:27 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/23 13:51:37 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "includes.h"

void	arg_error(std::string const & str)
{
	std::cerr << RED "Error: " << str << RESET << std::endl;
	exit(1);
}

void	arguments_parser(char **av)
{
	std::string	port = av[1];
	if (std::string(av[2]).empty() || port.empty())
		arg_error("wrong arguments.");
	for (std::string::iterator it = port.begin(); it != port.end(); it++)
	{
		if (!isdigit(*it))
			arg_error("invalid port.");
	}
	long long num = atol(av[1]);
	if (num < 1024 || num > 65535)
		arg_error("invalid port.");
}
