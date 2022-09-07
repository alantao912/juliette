#include "Queen.h"

Queen::Queen(Board::Color c, int8_t file, int8_t rank, Board *parent) : Piece(c, file, rank, parent) {}

void Queen::add_moves(std::vector<uint32_t> *move_list) {
    squares_hit = (uint64_t) 0;
    int8_t i = file;

    while (i > A_FILE) {
        --i;
        uint32_t move = create_move(i, rank, QUEEN);
        squares_hit |= (1ULL << Board::offset(i, rank));
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
        squares_hit |= (1ULL << Board::offset(i, rank));
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
        squares_hit |= (1ULL << Board::offset(file, i));
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
        squares_hit |= (1ULL << Board::offset(file, i));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }

    i = file;
    int8_t r = rank;
    while (i > A_FILE && r > 1) {
        --i;
        --r;
        uint32_t move = create_move(i, r, QUEEN);
        squares_hit |= (1ULL << Board::offset(i, r));
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
        squares_hit |= (1ULL << Board::offset(i, r));
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
        squares_hit |= (1ULL << Board::offset(i, r));
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
        squares_hit |= (1ULL << Board::offset(i, r));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
}

bool Queen::can_attack(int8_t file, int8_t rank) {
    return Rook::can_attack(this->file, this->rank, file, rank, parent) || Bishop::can_attack(this->file, this->rank, file, rank, parent);
}

char Queen::get_piece_char() {
    if (color == Board::BLACK) {
        return 'q';
    }
    return 'Q';
}

uint8_t Queen::get_type() const {
    return QUEEN;
}