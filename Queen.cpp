#include "Queen.h"

Queen::Queen(Board::Color c, uint8_t file, uint8_t rank, Board *parent) : Piece(c, file, rank, parent) {}

void Queen::add_moves(std::vector<uint32_t> *move_list) {
    squares_hit = (uint64_t) 0;
    uint8_t i = file;

    while (i > A_FILE) {
        --i;
        uint32_t move = create_move(i, rank, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, rank));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = file;
    while (i < H_FILE) {
        ++i;
        uint32_t move = create_move(i, rank, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, rank));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = rank;
    while (i > 1) {
        --i;
        uint32_t move = create_move(file, i, QUEEN);
        squares_hit |= (1ULL << parent->offset(file, i));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = rank;
    while (i < 8) {
        ++i;
        uint32_t move = create_move(file, i, QUEEN);
        squares_hit |= (1ULL << parent->offset(file, i));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }

    i = file;
    uint8_t r = rank;
    while (i > A_FILE && r > 1) {
        --i;
        --r;
        uint32_t move = create_move(i, r, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = file, r = rank;
    while (i > A_FILE && r < 8) {
        --i;
        ++r;
        uint32_t move = create_move(i, r, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = file, r = rank;
    while (i < H_FILE && r < 8) {
        ++i;
        ++r;
        uint32_t move = create_move(i, r, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
    i = file, r = rank;
    while (i < H_FILE && r > 1) {
        ++i;
        --r;
        uint32_t move = create_move(i, r, QUEEN);
        squares_hit |= (1ULL << parent->offset(i, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
}

bool Queen::can_attack(uint8_t file, uint8_t rank) {
    return Rook::can_attack(this->file, this->rank, file, rank, parent) || Bishop::can_attack(this->file, this->rank, file, rank, parent);
}

uint8_t Queen::get_piece_uint8_t() {
    if (color == Board::BLACK) {
        return 'q';
    }
    return 'Q';
}

uint8_t Queen::get_type() {
    return QUEEN;
}