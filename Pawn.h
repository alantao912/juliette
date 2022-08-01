#pragma once
#include <vector>
#include "Piece.h"
#include "Board.h"

class Pawn : public Piece {

private:
    void check_promotion(std::vector<uint32_t> *move_list, uint32_t move);

    uint8_t direction_offset, starting_rank, promotion_rank, en_passant_rank;

public:

    bool moved_two;

    Piece *promoted_piece;

    Pawn(Board::Color color, uint8_t file, uint8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(uint8_t file, uint8_t rank);

    uint8_t get_piece_uint8_t();

    uint8_t get_direction();

    uint8_t get_type();
};