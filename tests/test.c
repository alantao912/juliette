#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/resource.h>
#include "../src/board.h"
#include "../src/moves.h"
#include "../src/piece.h"

#define MAX_DEPTH 96

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

    for (int i = 0; i < reverted->num_uncaptured; ++i) {
        piece p = reverted->pieces[i];
        bool found = false;
        for (int j = 0; j < saved->num_uncaptured; ++j) {
            piece q = saved->pieces[j];
            if (p.file != q.file || p.rank != q.rank) {
                continue;
            }
            if (PIECE_TYPE(p.piece_data) != PIECE_TYPE(q.piece_data)) {
                continue;
            }
            found = true;
            break;
        }
        if (!found) {
            return false;
        }
    }

    for (int i = 0; i < saved->num_uncaptured; ++i) {
        piece p = saved->pieces[i];
        bool found = false;
        for (int j = 0; j < reverted->num_uncaptured; ++j) {
            piece q = reverted->pieces[j];
            if (p.file != q.file || p.rank != q.rank) {
                continue;
            }
            if (PIECE_TYPE(p.piece_data) != PIECE_TYPE(q.piece_data)) {
                continue;
            }
            found = true;
            break;
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

int main(int argc, char *argv[]) {
    bool cont = true;

    unsigned long long i = 0;
    while (cont) {
        board *bb = get_starting_position();
        time_t t;
        srand((unsigned int) t);
        board *saved[MAX_DEPTH];
        memset(saved, 0, sizeof(board *) * MAX_DEPTH);
        uint8_t curr_depth = 0;

        while (1) {
            if (curr_depth == MAX_DEPTH) {
                saved[curr_depth] = NULL;
                --curr_depth;
                uint16_t move = unmake_move(bb);
                if (!board_cmp(bb, saved[curr_depth])) {
                    printf("Failed to unmake move after cycles\n");
                    printf("Last move was ");
                    print_move(saved[curr_depth], move);
                    cont = false;
                    break;
                }
            } else if (curr_depth == 0){
                move_list *moves = generate_moves(bb);
                if (moves->size == 0) {
                    break;
                }
                uint8_t move_num = rand() % moves->size;
                uint16_t move = moves->moves[move_num];

                saved[curr_depth] = save_board(bb);
                make_move(bb, move);
                free(moves);
                ++curr_depth;
            } else if (rand() % 2 == 0) {
                saved[curr_depth] = NULL;
                --curr_depth;
                uint16_t move = unmake_move(bb);
                if (!board_cmp(bb, saved[curr_depth])) {
                    printf("Failed to unmake move after cycles\n");
                    printf("Last move was ");
                    print_move(saved[curr_depth], move);
                    cont = false;
                    break;
                }
            } else {
                move_list *moves = generate_moves(bb);
                if (moves->size == 0) {
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
        printf("Working %lld cycles!\n", i);
        for (int i = 0; i < MAX_DEPTH; ++i) {
            free(saved[i]);
        }
        free(bb);
        reset();
    }
    printf("Max memory usage was %lfMB.\n", get_mem_usage() / (1024.0 * 1024.0));
    return 0;
}