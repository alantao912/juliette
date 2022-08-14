#include "King.h"
const int8_t King::masks[8][2]= {{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}};

King::King(Board::Color c, int8_t file, int8_t rank, Board *parent) : Piece(c, file, rank, parent) {}

void King::add_moves(std::vector<uint32_t> *move_list) {
    squares_hit = (uint64_t) 0;
    for (const int8_t *offset : masks) { 
        int8_t f = file + offset[0], r = rank + offset[1];
        if (f < A_FILE || f > H_FILE) {
            continue;
        }
        if (r < 1 || r > 8) {
            continue;
        }
          
        uint32_t move = create_move(f, r, KING);
        squares_hit |= (1ULL << (parent->offset(f, r)));
        if (move == BREAK) {
            continue;
        }
        /* Only remove castling rights that it has. unmake_move() uses these flags to determine which castling rights to restore */
        if (short_castle_rights) {
            move = move | REM_SHORT_CASTLE;
        }

        if (long_castle_rights) {
            move = move | REM_LONG_CASTLE;
        }
        
        move_list->push_back(move);
    }
    if (short_castle_rights && can_castle_short()) {
        uint32_t move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file + 2)) | TO_RANK(rank) | SET_PIECE_MOVED(KING) | REM_SHORT_CASTLE | SHORT_CASTLING;
        if (long_castle_rights) {
            move = move | REM_LONG_CASTLE;
        }
        move_list->push_back(move);
    }

    if (long_castle_rights && can_castle_long()) {
        uint32_t move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file - 2)) | TO_RANK(rank) | SET_PIECE_MOVED(KING) | REM_LONG_CASTLE | LONG_CASTLING;
        if (short_castle_rights) {
            move = move | REM_SHORT_CASTLE;
        }
        move_list->push_back(move);
    }
}

bool King::can_castle_short() {
     std::vector<Piece *> *opposite_color_pieces = parent->get_opposite_pieces(color);
     Piece *p;
    
    if (!(p = parent->inspect(H_FILE, rank)) || !dynamic_cast< Rook *>(p) || p->color != this->color) {
        return false;
    }

    if ((p = parent->inspect(F_FILE, rank)) || (p = parent->inspect(G_FILE, rank))) {
        return false;
    }

    for (uint8_t i = 0; i < opposite_color_pieces->size(); ++i) {
        Piece *piece = opposite_color_pieces->at(i);
        if (piece->can_attack(F_FILE, rank)) {
            return false;
        }
    }
    return true;
}

bool King::can_castle_long() {
    std::vector<Piece *> *opposite_color_pieces = parent->get_opposite_pieces(color);
    Piece *p;
    
    if (!(p = parent->inspect(A_FILE, rank)) || !dynamic_cast<Rook *>(p) || p->color != this->color) {
        return false;
    }

    if ((p = parent->inspect(B_FILE, rank)) || (p = parent->inspect(C_FILE, rank)) || (p = parent->inspect(D_FILE, rank))) {
        return false;
    }

    for (uint8_t i = 0; i < opposite_color_pieces->size(); ++i) {
         Piece *piece = opposite_color_pieces->at(i);
        if (piece->can_attack(C_FILE, rank)) {
            return false;
        }
    }
    return true;
}

bool King::can_attack(int8_t file, int8_t rank) {
    return abs(file - this->file) <= 1 && abs(rank - this->rank) <= 1;
}

char King::get_piece_char() {
    if (color == Board::BLACK) {
        return 'k';
    }
    return 'K';
}

uint8_t King::hash_value() {
    return KING | (color * (1 << 3)) | (parent->move * (1 << 4)) | (long_castle_rights * (1 << 5)) | (short_castle_rights * (1 << 6));
}