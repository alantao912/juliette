//
// Created by Alan Tao on 9/4/2022.
//

#include "Zobrist.h"

static uint64_t zobrist_hashes[64 * 18];

static uint64_t black_to_move;

/*
ROOK_CAN_CASTLE_QUEEN 6
ROOK_CAN_CASTLE_KING  7
PAWN_JUST_MOVED_TWO   8
*/

void ZobristHashFunction::initialize() {
    for (unsigned long long & zobrist_hash : zobrist_hashes) {
        zobrist_hash = rand_bitstring();
    }
    black_to_move = rand_bitstring();
}

uint64_t ZobristHashFunction::rand_bitstring() {
    uint64_t hash = 0, mask = 1ULL;
    const uint64_t lim = 1ULL << 63;
    while (mask <= lim) {
        if (rand() % 2 == 0) {
            hash |= mask;
        }
        mask <<= 1;
    }
    return hash;
}

size_t ZobristHashFunction::operator()(const Board *game) {
    uint64_t hash = 0;
    if (game->turn == Board::BLACK) {
        hash ^= black_to_move;
    }

    for (const Piece *p : *(game->get_white_pieces())) {
        size_t i = 18 * game->offset(p->file, p->rank);
        King *king = game->white_king;
        uint8_t type = p->get_type();
        if (type == PAWN) {
            const Pawn *pawn = dynamic_cast<const Pawn *>(p);
            i += (4 * pawn->moved_two);
        } else if (type == ROOK) {
            i += 4 * (king->long_castle_rights && p->file == A_FILE) + 5 * (king->short_castle_rights && p->file == H_FILE);
        }
        i += type;
        hash ^= zobrist_hashes[i];
    }

    for (const Piece *p : *(game->get_black_pieces())) {
        size_t i = 18 * game->offset(p->file, p->rank);
        King *king = game->black_king;
        uint8_t type = p->get_type();
        if (type == PAWN) {
            const Pawn *pawn = dynamic_cast<const Pawn *>(p);
            i += (4 * pawn->moved_two);
        } else if (type == ROOK) {
            i += 4 * (king->long_castle_rights && p->file == A_FILE) + 5 * (king->short_castle_rights && p->file == H_FILE);
        }
        i += type;
        hash ^= zobrist_hashes[i];
    }
    return (size_t) hash;
}