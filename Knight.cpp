#include "Knight.h"

char Knight::masks[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, 2}, {1, -2}, {2, 1}, {2, -1}};

Knight::Knight(Board::Color c, char file, char rank, Board *parent) : Piece(c, file, rank, parent) {}

void Knight::add_moves(std::vector<uint32_t> *move_list) {
    for (char *offset : masks) {
        char f = file + offset[0], r = rank + offset[1];
        if (f < A_FILE || f > H_FILE) {
            continue;
        }

        if (r < 1 || r > 8) {
            continue;
        }
        uint32_t move = create_move(f, r, KNIGHT);
        if (move != BREAK) {
            move_list->push_back(move);
        }
    }
}

bool Knight::can_attack(char file, char rank) {
    char dx = abs(file - this->file);
    char dy = abs(rank - this->rank);
    return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
}

char Knight::get_piece_char() {
    if (color == Board::BLACK) {
        return 'n';
    }
    return 'N';
}

char Knight::get_type() {
    return KNIGHT;
}