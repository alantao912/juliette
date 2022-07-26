#include "Piece.h"
#include <iostream>

Piece::Piece(Board::Color c, char file, char rank, Board *p) : color(c),  parent(p) {
    this->file = file;
    this->rank = rank;
    this->is_taken = false;
}

uint32_t Piece::create_move(char to_file, char to_rank, char piece_moved) {
    Piece *p = parent->inspect(to_file, to_rank);
    uint32_t move = 0;
    if (!p) {
        move = move | FROM_FILE(this->file) | FROM_RANK (this->rank) | TO_FILE(to_file) | TO_RANK(to_rank) | SET_PIECE_MOVED(piece_moved);
    } else if (p->color != this->color) {
        move = move | FROM_FILE(this->file) | FROM_RANK(this->rank) | TO_FILE(to_file) | TO_RANK(to_rank) | IS_CAPTURE | SET_PIECE_MOVED(piece_moved);
    } else {
        move = BREAK;
    }
    return move;
}