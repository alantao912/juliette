#pragma once

#include <vector>
#include "Piece.h"
#include "Board.h"

class Bishop : public Piece {

private:
    static short middle_game_incentives[64];

public:

    Bishop(Board::Color c, uint8_t file, uint8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    static bool can_attack(uint8_t from_file, uint8_t from_rank, uint8_t to_file, uint8_t to_rank, Board *parent);

    bool can_attack(uint8_t file, uint8_t rank);

    uint8_t get_piece_uint8_t();

    uint8_t get_type();
};