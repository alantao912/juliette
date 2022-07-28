#pragma once
#include "Piece.h"
#include "Board.h"
#include "King.h"

class Rook : public Piece {

public:
    Rook(Board::Color c, char file, char rank, Board *parent);

    void add_moves(std::vector<uint32_t> *move_list);

    static bool can_attack(char from_file, char from_rank, char to_file, char to_rank, Board *parent);

    bool can_attack(char file, char rank);

    char get_piece_char();

    char get_type();

    short calculate_placement_value();
};