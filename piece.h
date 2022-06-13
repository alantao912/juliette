#ifndef PIECE_H
#define PIECE_H

#include "board.h"

#define EMPTY_SQUARE 0
#define PAWN 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define WHITE 1 << 3
#define BLACK 1 << 4

#define IS_BLACK(PIECE) ((PIECE & BLACK) != 0)
#define IS_WHITE(PIECE) ((PIECE & WHITE) != 0)
#define PIECE_TYPE(PIECE) (PIECE & 7)

#define CAN_CASTLE_SHORT(king) ((king & (1 << 5)) != 0)
#define SET_CASTLE_SHORT(king) (king = (king | (1 << 5)))
#define REM_CASTLE_SHORT(king) (king = (king & ~(1 << 5)))

#define CAN_CASTLE_LONG(king) ((king & (1 << 6)) != 0)
#define SET_CASTLE_LONG(king) (king = (king | (1 << 6)))
#define REM_CASTLE_LONG(king) (king = (king & ~(1 << 6)))

// Macro to be used for pieces defined as pawns only
#define PAWN_MOVED_TWO(pawn) (((1 << 6) & pawn) != 0)
#define SET_PAWN_MOVED_TWO(pawn) (pawn = (pawn | (1 << 6)))
#define REM_PAWN_MOVED_TWO(pawn) (pawn = (pawn & ~(1 << 6)))
/*
    ___ ___ ___ ___ ___ ___ ___ ___
     7   6   5   4   3   2   1   0

     bits 0 - 2 determine what type the pieece is
     bits 3 and 4 determine the color 4 being set to 1 is black, 3 being set to 1 is white
     bits 5 - 7 store miscellaneous info (has moved or not)

*/

char get_piece_char(board *bb, char file, char rank);

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

char get_piece_rep(char piece);

#endif 