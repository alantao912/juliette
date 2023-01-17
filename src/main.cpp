#include <cstdio>
#include <chrono>
#include <cstring>
#include <iostream>
#include <wspiapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "uci.h"
#include "stack.h"
#include "movegen.h"
#include "bitboard.h"

#pragma comment(lib, "Ws2_32.lib")

#undef UNICODE
#define BUFLEN 512
#define PORT "10531"
#define CONNECTION_FAILED 1

extern bitboard board;

void play_game() {
    init_board(START_POSITION);
    initialize_zobrist();
    std::string user_input;
    int depth;
    do {
        std::cout << "juliette:: Engine depth? ";
        try {
            std::cin >> user_input;
            trim(user_input);
            depth = std::stoi(user_input);
        } catch (const std::invalid_argument &arg) {
            depth = -1;
        }
    } while (depth <= 0);

    do {
        std::cout << "juliette:: Player Color? (w/b): ";
        std::cin >> user_input;
        trim(user_input);
    } while (user_input != "w" && user_input != "b");

    info_t reply;
    std::chrono::steady_clock::time_point start, end;
    if (user_input == "b") {
        start = std::chrono::steady_clock::now();
        reply = search(depth);
        end = std::chrono::steady_clock::now();
        make_move(reply.best_move);
    }

    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    bool player_turn = true;
    while (n) {
        system("cls");
        std::cout << "Elapsed Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << '\n';
        print_board();
        if (player_turn) {
            int i;
            for (i = 0; i < n; ++i) {
                std::cout << (i + 1) << ' ';
                print_move(moves[i]);
                std::cout << '\n';
            }

            do {
                std::cout << "juliette:: move? ";
                try {
                    std::cin >> user_input;
                    trim(user_input);
                    i = std::stoi(user_input);
                } catch (const std::invalid_argument &arg) {
                    i = -1;
                }
            } while (i < 1 || i > n);
            make_move(moves[i - 1]);
        } else {
            start = std::chrono::steady_clock::now();
            reply = search(depth);
            end = std::chrono::steady_clock::now();
            make_move(reply.best_move);
        }
        player_turn = !player_turn;
        n = gen_legal_moves(moves, board.turn);
    }
    if (is_check(board.turn)) {
        std::cout << "juliette:: Checkmate, ";
        if (player_turn) {
            std::cout << "computer wins!\n";
        } else {
            std::cout << "player wins!\n";
        }
    } else {
        std::cout << "juliette:: Stalemate, the game is drawn.\n";
    }
}

SOCKET listen(const char *port) {
    WSADATA wsaData;
    int iResult;

    auto listenSocket = INVALID_SOCKET;
    auto clientSocket = INVALID_SOCKET;

    struct addrinfo *result = nullptr;
    struct addrinfo hints{};
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout << "juliette:: WSAStartup() failed with error: " << iResult << std::endl;
        return CONNECTION_FAILED;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    /* Resolve the server address and port */
    iResult = getaddrinfo(nullptr, port, &hints, &result);
    if (iResult != 0) {
        std::cout << "juliette:: getaddrinfo() failed with error: " << iResult << std::endl;
        WSACleanup();
        return CONNECTION_FAILED;
    }

    /* Create a SOCKET for the server to listen for client connections. */
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "juliette:: socket() failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return CONNECTION_FAILED;
    }

    /* Setup the TCP listening socket */
    iResult = bind(listenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "juliette:: bind() failed with error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return CONNECTION_FAILED;
    }
    freeaddrinfo(result);

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "juliette:: listen() failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return CONNECTION_FAILED;
    } else {
        std::cout << "juliette:: listening for connections..." << std::endl;
    }

    /* Accept a client socket */
    clientSocket = accept(listenSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "juliette:: accept() failed with error: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return CONNECTION_FAILED;
    } else {
        std::cout << "juliette:: established connection!" << std::endl;
    }
    closesocket(listenSocket);
    return clientSocket;
}

enum input_source { REMOTE, STDIN};
input_source source;

/**
 * To compile: g++ *.cpp -lWS2_32 -o juliette
 */

int main(int argc, char *argv[]) {
    init_bishop_attacks();
    init_rook_attacks();
    _init_rays();
    if (argc == 1) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* If no options are provided */
        play_game();
        return 0;
    }

    char recvbuf[BUFLEN];
    enum comm_mode { UNDEFINED, UCI};
    comm_mode communication_mode = UNDEFINED;

    if (strcmp(argv[1], "remote") == 0) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        source = REMOTE;
        /* Engine is set to remote mode. Sending and receiving commands using sockets */
        while (true) {
            const char *port = PORT;
            if (argc >= 3 && strtol(argv[2], nullptr, 10)) {
                port = argv[2];
            }
            SOCKET clientSocket = listen(port);
            if (clientSocket == CONNECTION_FAILED) {
                std::cout << "juliette:: Internal engine error. Exiting ..." << std::endl;
                return -1;
            }
            int iResult;
            do {
                iResult = recv(clientSocket, recvbuf, BUFLEN, 0);
                recvbuf[iResult] = '\0';
                if (communication_mode == UCI) {
                    parse_UCI_string(recvbuf);
                } else if (strcmp(recvbuf, "uci") == 0) {
                    communication_mode = UCI;
                    initialize_UCI(clientSocket);
                    parse_UCI_string(recvbuf);
                }
                if (iResult == 0) {
                    std::cout << "juliette:: closing connection ..." << std::endl;
                    communication_mode = UNDEFINED;
                } else if (iResult == -1) {
                    std::cout << "juliette:: lost connection." << std::endl;
                    communication_mode = UNDEFINED;
                } else if (iResult < 0) {
                    std::cout << "juliette:: recv failed with error: " << (int) WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    WSACleanup();
                    return -1;
                }
            } while (iResult > 0);

            iResult = shutdown(clientSocket, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                std::cout << "juliette:: shutdown failed with error: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                return -1;
            }
            closesocket(clientSocket);
            WSACleanup();
        }
    } else if (strcmp(argv[1], "cli") == 0) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* Engine is set to CLI mode. Sending and receiving commands through stdout and stdin respectively. */
        source = STDIN;
        do {
            fgets(recvbuf, BUFLEN, stdin);
            if ((strlen(recvbuf) > 0) && (recvbuf[strlen (recvbuf) - 1] == '\n')) {
                recvbuf[strlen(recvbuf) - 1] = '\0';
            }
            if (communication_mode == UCI) {
                parse_UCI_string(recvbuf);
            } else if (strcmp(recvbuf, "uci") == 0) {
                communication_mode = UCI;
                initialize_UCI(0);
                parse_UCI_string(recvbuf);
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
                 init_board("1k2r1n1/pP1p4/6PP/p1R5/2n2P2/8/6N1/1BK3B1 w - - ");
                 print_board();
                 std::cout << '\n';
                 move_t moves[MAX_MOVE_NUM];
                 int n = gen_legal_moves(moves, board.turn);
                 order_moves(moves, n);

                 for (int i = 0; i < n; ++i) {
                     std::cout << (i + 1) << ". ";
                     print_move(moves[i]);
                     std::cout << ", " << moves[i].score;
                     std::cout << '\n';
                 }

            } else if (strcmp(recvbuf, "perft") == 0) {
                std::cout << "juliette:: starting performance test..." << std::endl;
                // TODO: Re-implement performance test
            } else if (strlen(recvbuf)) {
                std::cout << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)" << std::endl;
            }
        } while (strlen(recvbuf));
    } else if (strcmp(argv[1], "tune") == 0) {
        initialize_zobrist();
        init_board(START_POSITION);

        char *move_seq;
        if (argc < 3) {
            move_seq = new char;
            *move_seq = (char) 0;
        } else {
            move_seq = argv[2];
        }
        const std::size_t len = strlen(move_seq);
        std::size_t i = 0;

        move_t moves[MAX_MOVE_NUM];
        while (i < len) {
            const char src_file = move_seq[i] - 'a';
            const char src_rank = move_seq[i + 1] - '1';

            const char dest_file = move_seq[i + 2] - 'a';
            const char dest_rank = move_seq[i + 3] - '1';

            const char prom = move_seq[i + 4];

            int n = gen_legal_moves(moves, board.turn);
            int j;
            for (j = 0; j < n; ++j) {
                if (8 * src_rank + src_file == moves[j].from && 8 * dest_rank + dest_file == moves[j].to) {
                    if (prom == ' ') {
                        make_move(moves[j]);
                        break;
                    } else if (prom == 'q' && (moves[j].flag == PC_QUEEN || moves[j].flag == PR_QUEEN)) {
                        make_move(moves[j]);
                        break;
                    } else if (prom == 'r'&& (moves[j].flag == PC_ROOK || moves[j].flag == PR_ROOK)) {
                        make_move(moves[j]);
                        break;
                    } else if (prom == 'b' && (moves[j].flag == PC_BISHOP || moves[j].flag == PR_BISHOP)) {
                        make_move(moves[j]);
                        break;
                    } else if (prom == 'n' && (moves[j].flag == PC_KNIGHT || moves[j].flag == PR_KNIGHT)) {
                        make_move(moves[j]);
                        break;
                    }
                }
            }
            if (j == n) {
                std::cout << (int) src_file << (int) src_rank << (int) dest_file << (int) dest_rank << std::flush;
                return 0;
            }
            i += 5;
        }
        info_t reply = search(4);
        if (reply.best_move.from == A1 && reply.best_move.to == A1) {
            std::cout << "loss " << std::flush;
        } else if (reply.best_move.from == H8 && reply.best_move.to == H8) {
            std::cout << "draw " << std::flush;
        } else {
            print_move(reply.best_move);
        }
    }
    return 0;
}