#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"
#include "Rook.h"

class King : public Piece {
private:
    static char masks[8][2];

    bool can_castle_long();

public:

bool can_castle_short();

    bool long_castle_rights, short_castle_rights;

    King(Board::Color c, char file, char rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(char file, char rank);

    char get_piece_char();
};