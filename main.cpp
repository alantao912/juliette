#include <iostream>
#include "Board.h"
#include "Search.h"

void play_game() {
    Board *board = new Board("8/4k3/RR4K1/8/8/8/8/8 w -- --");
    while (true) {
        board->print_board();
        std::vector<uint32_t> *move_list = board->generate_moves();
        if (move_list->size() == 0) {
            if (board->is_king_in_check()) {
                std::cout << "Checkmate!" << std::endl;
                break;
            }
            std::cout << "Stalemate!" << std::endl;
            break;
        }

        for (int i = 0; i < move_list->size(); ++i) {
            std::cout << i + 1 <<  ". ";
            board->print_move(move_list->at(i));
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

int main(int argc, char *argv[]) {
    Board *b = new Board("6k1/R7/2Q5/8/8/4K3/8/8 w -- --");
    b->print_board();
    search(4);
    show_top_line();
    /*
    uint16_t port = 8080;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0), option = 1;
    if (!socket_fd) {
        std::cout << "Socket creation failed!" << std::endl;
        return -1;
    } else {
        std::cout << "Socket creation succeeded!" << std::endl;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof(option))) {
        std::cout << "Failed to set socket options" << std::endl;
        return -1;
    } else {
        std::cout << "Successfully set socket options!" << std::endl;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int result = bind(socket_fd, (struct sockaddr *) &address, sizeof(address));
    if (result < 0) {
        std::cout << "Socket binding failed!" << std::endl;
        return -1;
    } else {
        std::cout << "Successfully bound socket to port: " << port << '!' << std::endl;
    }
    */
    return 0;
}