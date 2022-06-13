#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define a 0
#define b 1
#define c 2
#define d 3
#define e 4
#define f 5
#define g 6
#define h 7

#define SQUARE(F, R) (((R - 1) * 8) + F)

/*

    [0: a1], [1: b1], [2: c1], [3: d1], [4: e1], [5: f1], [6: g1], [7: h1],
    [8: a2], [9: b2], [10: c2] ... [62: g8], [63: h8].

*/

typedef struct piece {
    char piece_data;
    char rank;
    char file;
} piece;

typedef struct board {
    char squares[64], num_uncaptured, move;
    piece pieces[32];
} board;

board *get_starting_position();

board *load_fen(char *fen);

void print_board(board *bb);

#endif