/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   includes.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 13:47:12 by roarslan          #+#    #+#             */
/*   Updated: 2025/08/16 10:49:27 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDES_H
 #define INCLUDES_H

#define RESET "\e[0m"
#define RED "\e[1;31m"
#define YELLOW "\e[1;33m"
#define GREEN "\e[1;32m"

#define BUFFER_SIZE 512

#define TIMEOUT_CHECK 10
#define PING_TIMEOUT 120
#define PING_CHECK 60

#include <iostream>
#include <vector>
#include <map>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <ctype.h>
#include <iomanip>
#include <set>
#include <csignal>
#include <string.h>

void	arguments_parser(char **av);
void	arg_error(std::string const & str);
extern	bool	g_running;
std::vector<std::string> splitIrc(const std::string & line);
bool	isValidNickname(const std::string &nickname);
std::vector<std::string>	splitList(const std::string &str);
bool	isValidChannelName(const std::string &str);

#endif
