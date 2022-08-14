#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"
#include "Rook.h"

class King : public Piece {
private:
    static const int8_t masks[8][2];

    bool can_castle_long();

    bool can_castle_short();

public:

    bool long_castle_rights, short_castle_rights;

    King(Board::Color c, int8_t file, int8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(int8_t file, int8_t rank);

    char get_piece_char();

    uint8_t hash_value();
};