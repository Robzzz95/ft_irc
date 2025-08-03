/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 14:25:27 by roarslan          #+#    #+#             */
/*   Updated: 2025/07/30 15:10:31 by roarslan         ###   ########.fr       */
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

std::vector<std::string> splitIrc(const std::string & line)
{
	std::vector<std::string> dest;
	std::string word;
	std::istringstream iss(line);

	while (iss >> word)
	{
		if (word[0] == ':')
		{
			std::string rest;
			std::getline(iss, rest);
			word += rest;
			dest.push_back(word);
			break ;
		}
		dest.push_back(word);
	}
	return (dest);
}

bool	isValidNickname(const std::string &nickname)
{
	if (nickname.empty() || nickname.size() > 9)
		return (false);

	char c = nickname[0];
	if (!isalpha(c) && std::string (" ,*?!@.:#[]\\`^{}").find(c) == std::string::npos)
		return (false);
	for (size_t i = 1; i < nickname.size(); i++)
	{
		c = nickname[i];
		if (!isalnum(c) && std::string(" ,*?!@.:#[]\\`^{}").find(c) == std::string::npos)
			return (false);
	}
	return (true);
}

std::vector<std::string>	splitList(const std::string &str)
{
	std::vector<std::string>	dest;
	std::string	tmp;

	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == ',')
		{
			if (!tmp.empty())
				dest.push_back(tmp);
			tmp.clear();
		}
		else
			tmp += str[i];
	}
	if (!tmp.empty())
		dest.push_back(tmp);	
	return (dest);
}
