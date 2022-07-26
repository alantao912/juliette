#pragma once
#include <vector>
#include "Piece.h"
#include "Board.h"

class Knight : public Piece {
private:
    static  char masks[8][2];

public:
    Knight(Board::Color c, char file, char rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(char file, char rank);

    char get_piece_char();

    char get_type();
};
