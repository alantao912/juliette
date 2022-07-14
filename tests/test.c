#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../src/board.h"
#include "../src/moves.h"
#include "../src/piece.h"

#define MAX_DEPTH 8

board *save_board(board *bb) {
    board *copy = (board *) malloc(sizeof(board));
    if (!copy) {
        return NULL;
    }
    memcpy(copy, bb, sizeof(board));
    return copy;
}

bool board_cmp(board *reverted, board *saved) {
    if (strncmp(reverted->squares, saved->squares, 64) != 0) {
        printf("Discrepancy in board data!\n");
        return false;
    }

    if (reverted->num_uncaptured != saved->num_uncaptured) {
        printf("Missing piece??? %d, %d\n", reverted->num_uncaptured, saved->num_uncaptured);
        return false;
    }

    for (char i = 0; i < reverted->num_uncaptured; ++i) {
        piece p = reverted->pieces[i];
        bool fm = false;
        for (char j = 0; j < saved->num_uncaptured; ++j) {
            if (strncmp((char *) &saved->pieces[j], (char *) &p, sizeof(piece)) == 0) {
                fm = true;
                break;
            }
        }
        if (!fm) {
            printf("Piece data from saved != Piece data from reverted %d\n", i);
            return false;
        }
    }

    for (char i = 0; i < saved->num_uncaptured; ++i) {
        piece p = saved->pieces[i];
        bool fm = false;
        for (char j = 0; j < reverted->num_uncaptured; ++j) {
            if (strncmp((char *) &reverted->pieces[j], (char *) &p, sizeof(piece)) == 0) {
                fm = true;
                break;
            }
        }
        if (!fm) {
            printf("Piece data from saved != Piece data from reverted %d\n", i);
            return false;
        }
    }
    return true;

}

int main(int argc, char *argv[]) {
    board *states[MAX_DEPTH];

    board *init_board = load_fen("7K/4P3/8/8/8/8/3p4/k7 w -- -- ");
    int depth = 0;

    time_t t;
    srand((unsigned) time(&t));
    int move_num = 1;

    #ifndef TEST
        while (true) {
            print_board(init_board);
            states[depth] = save_board(init_board);

            move_list *possible_moves = generate_moves(init_board);
            for (int i = 0; i < possible_moves->size; ++i) {
                printf("%d: ", i + 1);
                print_move(init_board, possible_moves->moves[i]);
            }
            scanf("%d", &move_num);

            if (move_num == 0) {
                uint16_t prev_move = unmake_move(init_board);
                if (!board_cmp(init_board, states[depth - 1])) {
                    printf("Failed to unmake move: ");
                    print_move(init_board, prev_move);
                    printf("\n\n");
                    print_board(init_board);
                    break;
                }
                --depth;
            } else {
                make_move(init_board, possible_moves->moves[move_num - 1]);
                ++depth;
            }
            system("clear");
        }
    #endif


    #ifdef TEST
        while (depth < MAX_DEPTH) {
            board *copy = save_board(init_board);
            states[depth] = copy;
            ++depth;

            move_list *possible_moves = generate_moves(init_board);
            if (possible_moves->size == 0) {
                printf("Out of moves\n");
                --depth;
                break;
            }
            int move_num = rand() % possible_moves->size;

            uint16_t move = possible_moves->moves[move_num];

            printf("%d.", depth);
            print_move(init_board, move);

            make_move(init_board, move);
            free(possible_moves);
        }
        bool failed = false;
        while (depth > 0) {
            uint16_t undone_move = unmake_move(init_board);

            // if (strncmp((char *) init_board, (char *) states[depth - 1], sizeof(board)) != 0) {
            if (!board_cmp(init_board, states[depth - 1])) {
                printf("On depth %d: Failed to unmake move: ", depth);
                print_move(init_board, undone_move);
                failed = true;
                break;
            }
            --depth;
        }
        if (!failed) {
            printf("Done!\n");
        } else {
            print_board(states[depth]);
        }
        return 0;
    #endif
}