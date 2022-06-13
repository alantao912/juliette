#ifndef MOVES_H
#define MOVES_H

#include <stddef.h>

#include "board.h"
#define OOM -1

#define SRC_FILE(file) file
#define SRC_RANK(rank) ((rank - 1) << 3)

#define GET_SRC_RANK(move) ((((7 << 3) & move) >> 3) + 1)
#define GET_SRC_FILE(move) (7 & move)

#define DEST_FILE(file) (file << 6)
#define DEST_RANK(rank) ((rank - 1) << 9)

#define GET_DEST_FILE(move)  (((7 << 6) & move) >> 6)
#define GET_DEST_RANK(move)  ((((7 << 9) & move) >> 9) + 1)

#define CAPTURED_PIECE(p) (p << 12)
#define GET_CAPTURED_PIECE(move) (((7 << 12) & move) >> 12)

#define IS_ENPASSANT() (1 << 15)
#define GET_IS_ENPASSANT(move) (((move >> 15) & 1) == 1)


/*
    ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___
     15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0   

    bits 0 - 2: File of piece to move.
    bits 3 - 5: Rank of piece to move.

    bits 6 - 8: File of destination square.
    bits 9 - 11: Rank of destination square.

    bits 12 - 14: Type of piece being captured.
    bit 15: is en-passant
*/

typedef struct move_list {
    unsigned int capacity, size;
    short *moves;
} move_list;

move_list *generate_moves(board *bb);

void print_move(board *bb, uint16_t move);

/**
 * @brief Pushes the move onto the move stack, and makes the necessary changes to the board data. 
 * 
 * @param bb   Pointer to board data. 
 * @param move Move to make
 */
void make_move(board *bb, uint16_t move);

/**
 * @brief Removes the top move from the move stack, and reverses the last move.
 * 
 * @param bb Pointer to board data.
 */

void unmake_move(board *bb);

#endif