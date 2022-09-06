//
// Created by Alan Tao on 9/4/2022.
//

#include "Zobrist.h"
#include "Pawn.h"
#include "King.h"

uint64_t zobrist_hashes[64 * 18];

uint64_t black_to_move;

/*
ROOK_CAN_CASTLE_QUEEN 6
ROOK_CAN_CASTLE_KING  7
PAWN_JUST_MOVED_TWO   8
*/

void initialize_zobrist() {
    for (unsigned long long & zobrist_hash : zobrist_hashes) {
        zobrist_hash = rand_bitstring();
    }
    black_to_move = rand_bitstring();
}

uint64_t rand_bitstring() {
    uint64_t hash = 0, mask = 1ULL;
    for (uint8_t i = 0; i < 64; ++i) {
        if (rand() % 2 == 0) {
            hash |= mask;
        }
        mask <<= 1;
    }
    return hash;
}

uint64_t fetch_bitstring(const Piece *p, const King *king) {
    size_t i = 18 * Board::offset(p->file, p->rank);
    uint8_t type = p->get_type();
    if (type == PAWN) {
        const Pawn *pawn = dynamic_cast<const Pawn *>(p);
        i += (4 * pawn->moved_two);
    } else if (type == ROOK) {
        i += 4 * (king->long_castle_rights && p->file == A_FILE) + 5 * (king->short_castle_rights && p->file == H_FILE);
    }
    i += type;
    return zobrist_hashes[i];
}

uint64_t hash(const Board *game) {
    uint64_t hash = 0;
    if (game->turn == Board::BLACK) {
        hash ^= black_to_move;
    }
    const King *king = game->white_king;
    for (const Piece *p : *(game->get_white_pieces())) {
        hash ^= fetch_bitstring(p, king);
    }

    king = game->black_king;
    for (const Piece *p : *(game->get_black_pieces())) {
        hash ^= fetch_bitstring(p, king);
    }
    return hash;
}