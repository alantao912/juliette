#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "piece.h"
#include "moves.h"

int main(int argc, char *argv[]) {
    board *init_state = load_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq --");
    system("clear");
    while (1) {
        print_board(init_state);
        move_list *ml = generate_moves(init_state);
        for (int i = 0; i < ml->size; ++i) {
            printf("%d. ", i + 1);
            print_move(init_state, ml->moves[i]);
        }

        int move_no;
        scanf("%d", &move_no);
        make_move(init_state, ml->moves[move_no - 1]);
        system("clear");
        free(ml);
    }
    return 0;
}