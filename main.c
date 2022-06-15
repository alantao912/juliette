#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "piece.h"
#include "moves.h"

extern char curr_move_depth;

int main(int argc, char *argv[]) {
    board *bb = get_starting_position();
    system("clear");
    while (1) {
        print_board(bb);
        move_list *ml = generate_moves(bb);
        for (int i = 0; i < ml->size; ++i) {
            printf("%d. ", i + 1);
            print_move(bb, ml->moves[i]);
        }

        int move_no;
        scanf("%d", &move_no);
        if (move_no != 0) {
            make_move(bb, ml->moves[move_no - 1]);
        } else if (curr_move_depth > 0) {
            unmake_move(bb);
        } else {
            printf("Cannot unmake move!\n");
        }

        system("clear");
        free(ml);
    }
    return 0;
}