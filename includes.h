/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   includes.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/12 13:47:12 by roarslan          #+#    #+#             */
/*   Updated: 2025/06/13 18:07:08 by roarslan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDES_H
 #define INCLUDES_H

#define RESET "\e[0m"
#define RED "\e[1;31m"
#define YELLOW "\e[1;33m"
#define GREEN "\e[1;32m"

#define BUFFER_SIZE 512

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

void	arguments_parser(char **av);
void	arg_error(std::string const & str);

#endif
