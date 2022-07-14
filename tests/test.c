#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/resource.h>
#include "../src/board.h"
#include "../src/moves.h"
#include "../src/piece.h"

#define MAX_DEPTH 64

long get_mem_usage() {
    struct rusage myusage;
    getrusage(RUSAGE_SELF, &myusage);
    return myusage.ru_maxrss;
}

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
        for (int i = 0; i < 64; ++i) {
            if (reverted->squares[i] != saved->squares[i]) {
                printf("Discrepancy on square %d: Reverted: %d, Saved: %d.\n", i, reverted->squares[i], saved->squares[i]);
                printf("Reverted board:\n");
                print_board(reverted);
                printf("\n\nSaved board:\n");
                print_board(saved);
            }
        }
        printf("strncmp failed!\n");
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    board *bb = get_starting_position();
    time_t t;
    srand((unsigned int) t);
    board *saved[MAX_DEPTH];
    uint8_t curr_depth = 0;
    // reverted data is not restoring the right to castle in all cases
    // certain piece data on the still have extraneous details set

    unsigned int i = 0;
    while (1) {
        if (curr_depth == MAX_DEPTH) {
            --curr_depth;
            uint16_t move = unmake_move(bb);
            if (!board_cmp(bb, saved[curr_depth])) {
                printf("Failed to unmake move after %d cycles\n", i);
                printf("Last move was ");
                print_move(saved[curr_depth], move);
                break;
            }
        } else if (curr_depth == 0){
            move_list *moves = generate_moves(bb);
            if (moves->size == 0) {
                printf("Out of moves after %d cycles!", i);
                // print_board(bb);
                break;
            }
            uint8_t move_num = rand() % moves->size;
            uint16_t move = moves->moves[move_num];

            saved[curr_depth] = save_board(bb);
            make_move(bb, move);
            free(moves);
            ++curr_depth;
        } else if (rand() % 2 == 0) {
            --curr_depth;
            uint16_t move = unmake_move(bb);
            if (!board_cmp(bb, saved[curr_depth])) {
                printf("Failed to unmake move after %d cycles\n", i);
                printf("Last move was ");
                print_move(saved[curr_depth], move);
                break;
            }
        } else {
            move_list *moves = generate_moves(bb);
            if (moves->size == 0) {
                printf("Out of moves after %d cycles!\n", i);
                // print_board(bb);
                break;
            }
            uint8_t move_num = rand() % moves->size;

            uint16_t move = moves->moves[move_num];

            saved[curr_depth] = save_board(bb);
            make_move(bb, move);
            free(moves);
            ++curr_depth;
        }
        ++i;
    }
    printf("Max memory usage %lfMB.\n", get_mem_usage() / (1024.0 * 1024.0));
    return 0;
}