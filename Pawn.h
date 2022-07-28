#pragma once
#include <vector>
#include "Piece.h"
#include "Board.h"

class Pawn : public Piece {

private:
    void check_promotion(std::vector<uint32_t> *move_list, uint32_t move);

    char direction_offset, starting_rank, promotion_rank, en_passant_rank;

public:

    bool moved_two;

    Piece *promoted_piece;

    Pawn(Board::Color color, char file, char rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    bool can_attack(char file, char rank);

    char get_piece_char();

    char get_direction();

    char get_type();

    short calculate_placement_value();
};