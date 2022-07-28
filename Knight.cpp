#include "Knight.h"

char Knight::masks[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, 2}, {1, -2}, {2, 1}, {2, -1}};

short Knight::middle_game_incentives[64] = {
    0 ,  0,   0 ,   0,   0,   0,   0,   0,
    0 , 20,  30 ,  30,  30,  30,  20,   0,
    0 , 20,  40 ,  40,  40,  40,  20,   0,
    0 ,  0,  20 ,  20,  20,  20,   0,   0,
   -20, -10, 20 ,  20,  20,  20, -10, -20,
   -20, -20,  0 ,   0,   0,   0, -20, -20,
   -20, -20, -10, -10, -10, -10, -20, -20,
   -30, -20, -20, -20, -20, -20, -20, -30
};

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
        *squares_hit = *squares_hit | (1 << parent->offset(f, r));
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

short Knight::calculate_placement_value() {
    if (parent->stage == Board::MIDDLEGAME) {
        char offset;
        if (color == Board::BLACK) {
            offset = parent->offset_invert_rank(this->file, this->rank);
        } else {
            offset = parent->offset(this->file, this->rank);
        }
        return middle_game_incentives[offset];
    }
    return 0;
}