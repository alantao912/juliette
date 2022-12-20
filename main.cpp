#include <cstdio>
#include <cstring>
#include <iostream>
#include <wspiapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "uci.h"
#include "coach.h"
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
    init_stack();
    move_t moves[MAX_MOVE_NUM];
    int n;
    do {
        n = gen_legal_moves(moves, board.turn);
        for (int i = 0; i < n; ++i) {
            move_t m = moves[i];
            std::cout << i + 1 << ") ";
            print_move(m);
            std::cout << '\n';
        }
        n = scanf("%d", &n);
        make_move(moves[n - 1]);
        std::cout << '\n';
    } while (n != 0);
    // TODO Re-implement vanilla chess gameplay with bitboards
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

/* g++ *.cpp *.c -lWS2_32 -o juliette */

int main(int argc, char *argv[]) {
    std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
    init_bishop_attacks();
    init_rook_attacks();
    _init_rays();
    if (argc == 1) {
        /* If no options are provided */
        play_game();
        return 0;
    }

    char recvbuf[BUFLEN];
    enum comm_mode { UNDEFINED, UCI};
    comm_mode communication_mode = UNDEFINED;

    if (strcmp(argv[1], "remote") == 0) {
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
                } else if (iResult == 10054) {
                    std::cout << "juliette:: client disconnected" << std::endl;
                } else if (iResult < 0) {
                    std::cout << "juliette:: recv failed with error: " << WSAGetLastError() << std::endl;
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
                init_board("8/4k3/8/8/8/6P1/5P1P/6K1 w - - ");
                print_bitboard(compute_king_vulnerabilities(board.w_king, board.w_pawns));
            } else if (strcmp(recvbuf, "perft") == 0) {
                std::cout << "juliette:: starting performance test..." << std::endl;
                // TODO: Re-implement performance test
            } else if (strlen(recvbuf)) {
                std::cout << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)" << std::endl;
            }
        } while (strlen(recvbuf));
    } else if (strcmp(argv[1], "coach") == 0) {
        coach trainer(argv[2]);

        trainer.save(argv[2]);
    }
    return 0;
}