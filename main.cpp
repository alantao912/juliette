#include <iostream>
#include "Evaluation.h"
#include "Board.h"
#include "King.h"

void play_game() {
    Board *board = new Board("8/6k1/8/8/3P4/5K2/8/8 w -- --");
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
        system("cls");
        delete move_list;
    }
}

int main(int argc, uint8_t *argv[]) {
    play_game();

    
    Board *pos = new Board("8/6k1/8/8/3P4/5K2/8/8 w -- --");
    delete pos->generate_moves();
    
    for (Piece *p : *(pos->get_white_pieces())) {
        std::cout << p->get_piece_uint8_t() << std::endl;
        
        uint64_t hit = p->squares_hit;
        uint64_t f = 0; 
        int r = 7, i = 0;
        while (i < 64) {
            int j = r * 8 + f;
            uint64_t mask = 1ULL << (j);
            
            if (mask & hit) {
                std::cout << "& ";
            } else {
                std::cout << "- ";
            }
            
            ++f;
            if (f == 8) {
                std::cout << '\n';
                --r;
                f = 0;
            }
            ++i;
        }
        
        std::cout << "\n\n";
    }
    
    return 0;
}