#include "Bishop.h"

Bishop::Bishop(Board::Color c, char file, char rank, Board *p) : Piece(c, file, rank, p) {}

void Bishop::add_moves(std::vector<uint32_t> *move_list) {
    char f = file, r = rank;
    while (f > A_FILE && r > 1) {
        --f;
        --r;
        uint32_t move = create_move(f, r, BISHOP);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    f = file, r = rank;
    while (f > A_FILE && r < 8) {
        --f;
        ++r;
        uint32_t move = create_move(f, r, BISHOP);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    f = file, r = rank;
    while (f < H_FILE && r < 8) {
        ++f;
        ++r;
        uint32_t move = create_move(f, r, BISHOP);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    f = file, r = rank;
    while (f < H_FILE && r > 1) {
        ++f;
        --r;
        uint32_t move = create_move(f, r, BISHOP);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
}

bool Bishop::can_attack(char from_file, char from_rank, char to_file, char to_rank, Board *parent) {
    if (abs(from_file - to_file) != abs(from_rank - to_rank)) {
        /* The two points here are not on the same diagonal*/
        return false;
    }
    char dx = 1 - (2 * (from_file > to_file));
    char dy = 1 - (2 * (from_rank > to_rank));

    to_file -= dx;
    to_rank -= dy;

    while (from_file != to_file || from_rank != to_rank) {
        from_file += dx;
        from_rank += dy;

        if (parent->inspect(from_file, from_rank)) {
            return false;
        }
    }
    return true;
}

bool Bishop::can_attack(char to_file, char to_rank) {
    return Bishop::can_attack(this->file, this->rank, file, rank, parent);
}

char Bishop::get_type() {
    return BISHOP;
}

char Bishop::get_piece_char() {
    if (color == Board::BLACK) {
        return 'b';
    }
    return 'B';
}