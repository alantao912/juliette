#include "Rook.h"

Rook::Rook(Board::Color c, char file, char rank, Board *parent) : Piece(c, file, rank, parent) {}

void Rook::add_moves(std::vector<uint32_t> *move_list) {
    char i = file;

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
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
    }
    i = file;
    while (i < H_FILE) {
        ++i;
        move = mask | create_move(i, rank, ROOK);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
    }
    i = rank;
    while (i > 1) {
        --i;
        move = mask | create_move(file, i, ROOK);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
    }
    i = rank;
    while (i < 8) {
        ++i;
        move = mask | create_move(file, i, ROOK);
        if (move == BREAK) {
            break;
        }
        move_list->push_back(move);
    }
}

bool Rook::can_attack(char from_file, char from_rank, char to_file, char to_rank, Board *parent) {
    char dx = (to_file > from_file) - (to_file < from_file);
    char dy = (to_rank > from_rank) - (to_rank < from_rank);

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

bool Rook::can_attack(char dest_file, char dest_rank) {
    return Rook::can_attack(this->file, this->rank, dest_file, dest_rank, parent);
}

char Rook::get_piece_char() {
    if (color == Board::BLACK) {
        return 'r';
    }
    return 'R';
}

char Rook::get_type() {
    return ROOK;
}