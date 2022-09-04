#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"
#include "Bishop.h"
#include "Rook.h"

class Queen : public Piece {

public:
    Queen(Board::Color c, int8_t file, int8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list) override;

    bool can_attack(int8_t file, int8_t rank) override;

    char get_piece_char() override;

    uint8_t get_type() const override;
};