# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: roarslan <roarslan@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/11 16:21:43 by roarslan          #+#    #+#              #
#    Updated: 2025/08/16 13:37:36 by roarslan         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv
BOT_NAME = ircbot

CPP = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp \
	utils.cpp \
	Server.cpp \
	Client.cpp \
	Channel.cpp

BOT_SRC = Bot/mainBot.cpp \
	Bot/Bot.cpp

OBJDIR = ./obj/
OBJ = $(SRC:%.cpp=$(OBJDIR)%.o)
BOT_OBJ = $(BOT_SRC:%.cpp=$(OBJDIR)%.o)

all: $(NAME) $(BOT_NAME)

$(NAME): $(OBJ)
	$(CPP) $(OBJ) $(CPPFLAGS) -o $(NAME)

$(BOT_NAME): $(BOT_OBJ)
	$(CPP) $(BOT_OBJ) $(CPPFLAGS) -o $(BOT_NAME)

$(OBJDIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BOT_OBJ)

fclean: clean
	rm -f $(NAME) $(BOT_NAME)

re: fclean all

# Default values for server and bot configuration
SERVER_PORT ?= 6667
SERVER_PASS ?= pope

BOT_IP ?= 127.0.0.1
BOT_PORT ?= $(SERVER_PORT)
BOT_PASS ?= $(SERVER_PASS)
BOT_NICK ?= pope

run-server:
	./$(NAME) $(SERVER_PORT) $(SERVER_PASS)

run-bot: $(BOT_NAME)
	./$(BOT_NAME) $(BOT_IP) $(BOT_PORT) $(BOT_PASS) $(BOT_NICK)

# Run this in a separate terminal after the server is running
run: run-bot

.PHONY: clean fclean re all run run-server run-bot