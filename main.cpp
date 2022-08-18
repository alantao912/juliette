#include <iostream>
#include "Evaluation.h"
#include "Board.h"
#include "Bishop.h"
#include "King.h"

void play_game() {
    Board *board = new Board("rnbq1bnr/ppppk1pp/8/7Q/3pP3/8/PPP2PPP/RNB1KB1R w KQ --");
    while (true) {
        board->print_board();
        std::vector<uint32_t> *move_list = board->generate_moves();
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
        // system("cls");
        delete move_list;
    }
    delete board;
}

int main(int argc, char *argv[]) {
    play_game();
    return 0;
}