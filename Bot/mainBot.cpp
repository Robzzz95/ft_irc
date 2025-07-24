#include "Bot.hpp"
#include "Server.hpp"

Bot* globalBot = NULL;
volatile bool shouldClose = false;

void Bot::closeConnection() {
    std::vector<std::string>().swap(_quotes);
    handleQuit(_serSocketBot,getClientByName(_nick), true);
}

void signalHandler(int signal) {
    (void)signal;
    if (globalBot) {
        std::cout << "Signal received. Cleaning up bot." << std::endl;
        globalBot->closeConnection();
        shouldClose = true;
        exit (0);
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> <pass>" << std::endl;
        return 1;
    }

    try {
        Bot bot(atoi(argv[2]), argv[1], "FiceloBot", "FiceloBot", argv[3]);
        globalBot = &bot;

        signal(SIGINT, signalHandler);
        signal(SIGQUIT, signalHandler);
        bot.loadQuotes("srcs/Bot/quotes.txt");
        bot.botInit();
        bot.listenToServer();

        if (shouldClose) {
            std::cout << "Shutting down bot gracefully." << std::endl;
        }

        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

/****
 * Bot doit prendre ip, port, pass, nick, user en argument
 * Msg de confirmation de connexion
 * Client = /bot, le bot doit pouvoir rejoindre le channel la ou le client a execute la commande /bot
 * Bot ouvre quotes.txt et envoie une quote aleatoire
 * channel #test : ficelo !quotes bot doit pouvoir sortir une quote dans le channel #test
*/

