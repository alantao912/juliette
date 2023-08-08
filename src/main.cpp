#include <iostream>

#include "uci.h"
#include "stack.h"
#include "movegen.h"
#include "bitboard.h"

#define BUFLEN 512

extern __thread bitboard board;

/**
 * To compile: g++ src/*.cpp -lpthread -o juliette
 *
 * ./juliette cli
 */

int main(int argc, char *argv[]) {
    init_bishop_attacks();
    init_rook_attacks();
    _init_rays();
    if (argc == 1) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* If no options are provided */

        return 0;
    }

    char recvbuf[BUFLEN];
    enum comm_mode {
        UNDEFINED, UCI
    };
    comm_mode communication_mode = UNDEFINED;

    if (strcmp(argv[1], "cli") == 0) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* Engine is set to CLI mode. Sending and receiving commands through stdout and stdin respectively. */
        do {
            std::cin.getline(recvbuf, BUFLEN);
            if ((strlen(recvbuf) > 0) && (recvbuf[strlen(recvbuf) - 1] == '\n')) {
                recvbuf[strlen(recvbuf) - 1] = '\0';
            }
            if (communication_mode == UCI) {
                UCI::parse_UCI_string(recvbuf);
            } else if (strcmp(recvbuf, "uci") == 0) {
                communication_mode = UCI;
                UCI::initialize_UCI();
                UCI::parse_UCI_string(recvbuf);
            } else if (strcmp(recvbuf, "comm") == 0) {
                std::cout << "juliette:: to select a communication protocol, enter it's name:" << std::endl;
                std::cout << "uci" << std::endl;
                std::cout << "dev (developer use)" << std::endl;
                std::cout << "perft" << std::endl;
            } else if (strcmp(recvbuf, "dev") == 0) {
                std::cout << "juliette:: switched to development mode." << std::endl;
                /**
                 * To be used for development purposes:
                 */
                initialize_zobrist();
                UCI::parse_UCI_string("uci");
                UCI::parse_UCI_string("ucinewgame");
                UCI::parse_UCI_string("position rnbqkbnr/pp3ppp/2p5/3pp3/3P4/2N5/PPP1PPPP/R1BQKBNR b KQkq d3 0 4 ");
                UCI::parse_UCI_string("go");

            } else if (strcmp(recvbuf, "perft") == 0) {
                std::cout << "juliette:: starting performance test..." << std::endl;
                // TODO: Re-implement performance test
            } else if (strlen(recvbuf)) {
                std::cout
                        << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)"
                        << std::endl;
            }
        } while (strlen(recvbuf));
    } else {
        std::cout << "juliette:: please provide at least one argument." << std::endl;
    }
    return 0;
}