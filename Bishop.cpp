#include "Bishop.h"

Bishop::Bishop(Board::Color c, int8_t file, int8_t rank, Board *p) : Piece(c, file, rank, p) {}

void Bishop::add_moves(std::vector<uint32_t> *move_list) {
    squares_hit = (uint64_t) 0;
    int8_t f = file, r = rank;
    while (f > A_FILE && r > 1) {
        --f;
        --r;
        uint32_t move = create_move(f, r, BISHOP);
        squares_hit |= (1ULL << parent->offset(f, r));
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
        squares_hit |= (1ULL << parent->offset(f, r));
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
        squares_hit |= (1ULL << parent->offset(f, r));
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
        squares_hit |= (1ULL << parent->offset(f, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
}

bool Bishop::can_attack(int8_t from_file, int8_t from_rank, int8_t to_file, int8_t to_rank, Board *parent) {
    if (abs(from_file - to_file) != abs(from_rank - to_rank)) {
        /* The two points here are not on the same diagonal*/
        return false;
    }
    int8_t dx = 1 - (2 * (from_file > to_file));
    int8_t dy = 1 - (2 * (from_rank > to_rank));

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

bool Bishop::can_attack(int8_t to_file, int8_t to_rank) {
    return Bishop::can_attack(this->file, this->rank, to_file, to_rank, parent);
}

uint8_t Bishop::get_type() const {
    return BISHOP;
}

char Bishop::get_piece_char() {
    if (color == Board::BLACK) {
        return 'b';
    }
    return 'B';
}