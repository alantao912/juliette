#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"
#include "Bishop.h"
#include "Rook.h"

class Queen : public Piece {

public:
    Queen(Board::Color c, uint8_t file, uint8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(uint8_t file, uint8_t rank);

    uint8_t get_piece_uint8_t();

    uint8_t get_type();
};