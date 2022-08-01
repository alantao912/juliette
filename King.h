#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"
#include "Rook.h"

class King : public Piece {
private:
    static uint8_t masks[8][2];

    bool can_castle_long();

public:

bool can_castle_short();

    bool long_castle_rights, short_castle_rights;

    King(Board::Color c, uint8_t file, uint8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(uint8_t file, uint8_t rank);

    uint8_t get_piece_uint8_t();
};