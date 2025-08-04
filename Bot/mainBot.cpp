/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mainBot.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sacha <sacha@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/29 15:37:14 by sacha             #+#    #+#             */
/*   Updated: 2025/07/29 15:37:14 by sacha            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Bot.hpp"
#include <iostream>
#include <csignal>

Bot* globalBot = nullptr;

void signalHandler(int signal) {
    (void)signal;
    if (globalBot) {
        delete globalBot;
        globalBot = nullptr;
    }
    exit(0);
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> <pass> <nick>" << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        // Créer le bot avec les paramètres reçus et des valeurs par défaut pour user et realname
        Bot* bot = new Bot(argv[1], argv[2], argv[3], argv[4], argv[4], "Bot Modérateur");
        globalBot = bot;

        if (!bot->connectToServer()) {
            std::cerr << "Erreur de connexion au serveur IRC." << std::endl;
            delete bot;
            return 1;
        }

        bot->loadBadWords("Bot/badwords.txt");
        bot->listenToServer();

        delete bot;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 1;
    }
}
