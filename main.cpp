#include <iostream>
#include "Board.h"
#include "Evaluation.h"
#include "Search.h"

void play_game() {
    Board *board = new Board(START_POSITION);
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
        std::cout << "Evaluation: " << evaluate(board) << std::endl;

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
    // play_game();
    Board *board = new Board("8/3r4/6k1/8/8/3R1K2/8/8 w -- --");
    board->print_board();
    uint32_t move = search(board, 2);
    std::cout << "Best move: ";
    board->print_move(move);
    return 0;
}