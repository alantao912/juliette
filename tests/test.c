#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/board.h"
#include "../src/moves.h"
#include "../src/piece.h"

#define MAX_DEPTH 64

board *save_board(board *bb) {
    board *copy = (board *) malloc(sizeof(board));
    if (!copy) {
        return NULL;
    }
    memcpy(copy, bb, sizeof(board));
    return copy;
}

int main(int argc, char *argv[]) {
    /*
    board *bb = load_fen("2rqk1n1/p1n3pr/3bp2p/1Ppp4/2P2P2/BPN4P/3PPP1R/R2Q1KN1 b -- -- ");
    print_board(bb);
    move_list *moves = generate_moves(bb);

    for (int i = 0; i < moves->size; ++i) {
        print_move(bb, moves->moves[i]);
    }

    return 0;
    */
    
    board *states[MAX_DEPTH];

    board *init_board = get_starting_position();
    int depth = 0;

    time_t t;
    srand((unsigned) time(&t));

    while (depth < MAX_DEPTH) {
        board *copy = save_board(init_board);
        states[depth] = copy;
        ++depth;

        move_list *possible_moves = generate_moves(init_board);
        int move_num = rand() % possible_moves->size;
        
        uint16_t move = possible_moves->moves[move_num];
        print_move(init_board, move);

        make_move(init_board, move);
        free(possible_moves);
    }
    print_board(init_board);
    return 0;
    
}