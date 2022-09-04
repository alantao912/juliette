#pragma once
#include <vector>
#include "Piece.h"
#include "Board.h"

class Pawn : public Piece {

private:
    void check_promotion(std::vector<uint32_t> *move_list, uint32_t move);

    int8_t direction_offset, starting_rank, promotion_rank, en_passant_rank;

public:

    bool moved_two;

    Piece *promoted_piece;

    Pawn(Board::Color color, int8_t file, int8_t rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list) override;

    bool can_attack(int8_t file, int8_t rank) override;

    char get_piece_char() override;

    int8_t get_direction();

    uint8_t get_type() const override;
};