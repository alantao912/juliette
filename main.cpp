#include <iostream>
#include "Evaluation.h"
#include "Board.h"
#include "King.h"

int main(int argc, char *argv[]) {
    Board *board = new Board(START_POSITION);
    initialize_evaluators();
    while (true) {
        board->print_board();
        std::cout << "Evaluation: " << evaluate(board) << std::endl;
        std::vector<uint32_t> *move_list = board->generate_moves();

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
    return 0;
}