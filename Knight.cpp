#include "Knight.h"

const int8_t Knight::masks[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, 2}, {1, -2}, {2, 1}, {2, -1}};

Knight::Knight(Board::Color c, int8_t file, int8_t rank, Board *parent) : Piece(c, file, rank, parent) {}

void Knight::add_moves(std::vector<uint32_t> *move_list) {
    squares_hit = (uint64_t) 0;
    for (const int8_t *offset : masks) {
        int8_t f = file + offset[0], r = rank + offset[1];
        if (f < A_FILE || f > H_FILE) {
            continue;
        }

        if (r < 1 || r > 8) {
            continue;
        }
        uint32_t move = create_move(f, r, KNIGHT);
        squares_hit |= (1ULL << parent->offset(f, r));
        if (move != BREAK) {
            move_list->push_back(move);
        }
    }
}

bool Knight::can_attack(int8_t file, int8_t rank) {
    int8_t dx = abs(file - this->file);
    int8_t dy = abs(rank - this->rank);
    return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
}

char Knight::get_piece_char() {
    if (color == Board::BLACK) {
        return 'n';
    }
    return 'N';
}

uint8_t Knight::get_type() {
    return KNIGHT;
}

uint8_t Knight::hash_value() {
    return KNIGHT | (color * (1 << 3)) | (parent->move * (1 << 4));
}