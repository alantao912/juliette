#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include<vector>
#include <unordered_map>
#include <ctime>

#include "Board.h"
#include "UCI.h"
#include "Zobrist.h"

#pragma comment(lib, "Ws2_32.lib")
#undef UNICODE
#define BUFLEN 512
#define PORT "10531"
#define CONNECTION_FAILED 1

void play_game() {
    auto *board = new Board("8/4k3/RR4K1/8/8/8/8/8 w -- --");
    while (true) {
        board->print_board();
        std::vector<uint32_t> *move_list = board->generate_moves();
        if (move_list->empty()) {
            if (board->is_king_in_check()) {
                std::cout << "Checkmate!" << std::endl;
                break;
            }
            std::cout << "Stalemate!" << std::endl;
            break;
        }

        for (int i = 0; i < move_list->size(); ++i) {
            std::cout << i + 1 <<  ". ";
            Board::print_move(move_list->at(i));
            std::cout << std::endl;
        }

        int move_num;
        scanf("%d", &move_num);
        if (move_num == 0) {
            board->revert_move();
            
        } else if (move_num > move_list->size()) {
            std::cout << "Please enter a number < " << move_list->size() + 1 << std::endl;
        } else {
            board->make_move(move_list->at(move_num - 1));
        }
        system("cls");
        delete move_list;
    }
    delete board;
}

SOCKET listen() {
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
    iResult = getaddrinfo(nullptr, PORT, &hints, &result);
    if ( iResult != 0 ) {
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

int main(int argc, char *argv[]) {
    std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
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
            SOCKET clientSocket = listen();
            if (clientSocket == CONNECTION_FAILED) {
                std::cout << "juliette:: Internal server error. Exiting ..." << std::endl;
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
                std::cout << "exp (developer use)" << std::endl;
            } else if (strcmp(recvbuf, "exp") == 0) {
                std::cout << "juliette:: switched to experimental mode." << std::endl;
                initialize_zobrist();
                auto *b = new Board("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR w -- --");
                int i = 0;
                srand(time(NULL));
                while (true) {
                    auto *move_list = b->generate_moves();
                    uint32_t move = move_list->at(rand() % move_list->size());
                    uint64_t batch_hash_code = hash(b);
                    uint64_t incremental_hash_code = b->hash_code;
                    if (batch_hash_code != incremental_hash_code) {
                        std::cout << batch_hash_code << ", " << incremental_hash_code << " " << i << std::endl;
                        break;
                    }
                    Board::print_move(move);
                    std::cout << '\n';
                    b->make_move(move);
                    delete move_list;
                    ++i;
                }
            } else {
                std::cout << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)" << std::endl;
            }

        } while (strlen(recvbuf));
    }
    return 0;
}