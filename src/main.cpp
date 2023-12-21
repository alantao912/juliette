#include <iostream>

#include "uci.h"
#include "movegen.h"

#define BUFLEN 512

/**
 * To compile: 
 *  g++ src/*.cpp -lpthread -o juliette
 * 
 * To run:
 *  ./juliette cli
 */

enum CommunicationMode {
    UNDEFINED, UniformChessInterface
};

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "juliette:: \"Please select input mode.\"" << std::endl;
        return 0;
    }

    MoveGen::initMoveGenData();

    CommunicationMode mode = CommunicationMode::UNDEFINED;
    char recvbuf[BUFLEN];
    if (strcmp(argv[1], "cli") == 0) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;

        // Engine is set to CLI mode. Sending and receiving commands through stdout and stdin.
        UCI io;
        SearchContext::SetUciInstance(&io);
        
        
        do {
            std::cin.getline(recvbuf, BUFLEN);

            size_t inputLen = strlen(recvbuf);
            if ((inputLen > 0) && (recvbuf[inputLen - 1] == '\n')) {
                recvbuf[inputLen - 1] = '\0';
            }

            if (mode == CommunicationMode::UniformChessInterface) {
                io.parseUCIString(recvbuf);
            } else if (strcmp(recvbuf, "uci") == 0) {
                mode = CommunicationMode::UniformChessInterface;
                io.parseUCIString(recvbuf);
            } else if (strcmp(recvbuf, "comm") == 0) {
                std::cout << "juliette:: to select a communication protocol, enter it's name:" << std::endl;
                std::cout << "uci" << std::endl;
                std::cout << "dev (developer use)" << std::endl;
                std::cout << "perft" << std::endl;
            } else if (strcmp(recvbuf, "dev") == 0) {
                std::cout << "juliette:: switched to development mode." << std::endl;
                //
                // To be used for development purposes:
                //

                // [insert dev code here]
                /*
                move_t moves[Bitboard::MAX_MOVE_NUM];
                Bitboard board(START_POSITION);
                while (1) {
                    board.printBoard();
                    int n = board.genLegalMoves(moves, board.getTurn());
                    for (int i = 0; i < n; ++i) {
                        std::cout << i + 1 << ". ";
                        IOUtils::printMove(moves[i]);
                        std::cout << '\n';
                    }
                    int i;
                    std::cin >> i;
                    board.makeMove(moves[i - 1]);
                }
                */
               io.parseUCIString("uci");
               io.parseUCIString("setoption name threadCount value 1");
               io.parseUCIString("ucinewgame");
               io.parseUCIString("position startpos");
               io.parseUCIString("go movetime 2000");
            } else if (strcmp(recvbuf, "perft") == 0) {
                std::cout << "juliette:: starting performance test..." << std::endl;
                //
                // To be used for performance test:
                //

                // [insert performance test code here]
            } else if (strlen(recvbuf)) {
                std::cout << "Unrecognized command: " << recvbuf << '\n';
                std::cout
                        << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)"
                        << std::endl;
            }
        } while (strlen(recvbuf));
    }
    return 0;
}