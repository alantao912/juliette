#include "Rook.h"

Rook::Rook(Board::Color c, int8_t file, int8_t rank, Board *parent) : Piece(c, file, rank, parent) {}

void Rook::add_moves(std::vector<uint32_t> *move_list) {
    int8_t i = file;

    uint32_t move, mask = 0;
    King *my_king = parent->get_my_king(this->color);
    if (rank == 1 && file == A_FILE && my_king->long_castle_rights) {
        mask = REM_LONG_CASTLE;
    } else if (rank == 1 && file == H_FILE && my_king->short_castle_rights) {
        mask = REM_SHORT_CASTLE;
    }
    while (i > A_FILE) {
        --i;
        move = mask | create_move(i, rank, ROOK);
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
        move = mask | create_move(i, rank, ROOK);
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
        move = mask | create_move(file, i, ROOK);
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
        move = mask | create_move(file, i, ROOK);
        squares_hit |= (1ULL << parent->offset(file, i));
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
        if (GET_IS_CAPTURE(move)) {
            break;
        }
    }
}

bool Rook::can_attack(int8_t from_file, int8_t from_rank, int8_t to_file, int8_t to_rank, Board *parent) {
    int8_t dx = (to_file > from_file) - (to_file < from_file);
    int8_t dy = (to_rank > from_rank) - (to_rank < from_rank);

    if (dx != 0 && dy != 0) {
        return false;
    }

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

bool Rook::can_attack(int8_t dest_file, int8_t dest_rank) {
    return Rook::can_attack(this->file, this->rank, dest_file, dest_rank, parent);
}

char Rook::get_piece_char() {
    if (color == Board::BLACK) {
        return 'r';
    }
    return 'R';
}

uint8_t Rook::get_type() {
    return ROOK;
}

uint8_t Rook::hash_value() {
    return ROOK | (color * (1 << 3)) | (parent->move * (1 << 4));
}