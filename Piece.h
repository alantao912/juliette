#pragma once

#define BREAK UINT32_MAX
#define FROM_FILE(file) file
#define FROM_RANK(rank) ((rank - 1) << 3)

#define TO_FILE(file) (file << 6)
#define TO_RANK(rank) ((rank - 1) << 9)

#define GET_FROM_FILE(move) (7 & move)
#define GET_FROM_RANK(move) ((((7 << 3) & move) >> 3) + 1)

#define GET_TO_FILE(move) (((7 << 6) & move) >> 6)
#define GET_TO_RANK(move) ((((7 << 9) & move) >> 9) + 1)

#define IS_CAPTURE 1 << 12
#define GET_IS_CAPTURE(move) ((move & (1 << 12)) != 0)

#define IS_ENPASSANT 1 << 13
#define GET_IS_ENPASSANT(move) ((move & (1 << 13)) != 0)

#define IS_PROMOTION 1 << 14
#define GET_IS_PROMOTION(move) ((move & (1 << 14)) != 0)

#define KNIGHT 0
#define BISHOP 1
#define ROOK 2
#define QUEEN 3
#define PAWN 4
#define KING 5

#define PROMOTION_PIECE(piece) (piece << 15)
#define GET_PROMOTION_PIECE(move) ((move & (3 << 15)) >> 15)

#define REM_SHORT_CASTLE 1 << 17
#define GET_REM_SCASTLE(move) ((move & (1 << 17)) != 0)

#define REM_LONG_CASTLE 1 << 18
#define GET_REM_LCASTLE(move) ((move & (1 << 18)) != 0)

#define LONG_CASTLING 1 << 19
#define GET_LONG_CASTLING(move) ((move & (1 << 19)) != 0)

#define SHORT_CASTLING 1 << 20
#define GET_SHORT_CASTLING(move) ((move & (1 << 20)) != 0)

#define IS_CHECK 1 << 21
#define GET_IS_CHECK(move) ((move & (1 << 21)) != 0)

#define SET_PIECE_MOVED(piece) (piece << 29)
#define GET_PIECE_MOVED(move) ((move & (7 << 29)) >> 29)

/**
 * __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __
 * 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
 */

class Board;

#include "Board.h"

class Piece {

protected:
    Board *parent;

    uint32_t create_move(int8_t to_file, int8_t to_rank, uint8_t piece_moved);

public:

    uint64_t squares_hit;

    int8_t file, rank;

    bool is_taken;

    Board::Color color;

    Piece(Board::Color color, int8_t file, int8_t rank, Board *parent);

    virtual void add_moves(std::vector<uint32_t> *move_list) {};

    virtual bool can_attack(int8_t file, int8_t rank)  { return false; };

    virtual char get_piece_char() { return ' ';};

    virtual uint8_t get_type() {return KING;};

    virtual uint8_t hash_value() { return 0;}
};