#pragma once
#include <vector>
#include "Piece.h"
#include "Board.h"

class Knight : public Piece {
private:
    static const int8_t masks[8][2];

public:
    Knight(Board::Color c, int8_t file, int8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(int8_t file, int8_t rank);

    char get_piece_char();

    uint8_t get_type();

    uint8_t hash_value();
};
