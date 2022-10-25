#include "evaluation.h"
#include "movegen.h"

/* Pointer to board object that we are currently evaluating */
extern bitboard board;

uint8_t knight_hitboard[] = {

};

uint64_t w_pawn_attacks, b_pawn_attacks;

int32_t midgame_score, endgame_score;

inline uint64_t reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

int32_t evaluate() {
    material_score();
    w_pawn_attacks = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H));
    b_pawn_attacks = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A));
    pawn_structure();
    pawn_progression();
    doubled_pawns();
    knight_activity();
    bishop_activity();
    rook_activity();
    queen_activity();
    return (1 - 2 * (board.turn == BLACK)) * (midgame_score);
}

inline void material_score() {
    int n = pop_count(board.w_pawns);
    midgame_score += n * PAWN_MATERIAL;
    endgame_score += n * PAWN_MATERIAL_E;
    n = pop_count(board.w_knights);
    midgame_score += n * KNIGHT_MATERIAL;
    endgame_score += n * KNIGHT_MATERIAL_E;
    n = pop_count(board.w_bishops);
    midgame_score += n * BISHOP_MATERIAL;
    endgame_score += n * BISHOP_MATERIAL_E;
    n = pop_count(board.w_rooks);
    midgame_score += n * ROOK_MATERIAL;
    endgame_score += n * ROOK_MATERIAL_E;
    n = pop_count(board.w_queens);
    midgame_score += n * QUEEN_MATERIAL;
    endgame_score += n * QUEEN_MATERIAL_E;

    n = pop_count(board.b_pawns);
    midgame_score -= n * PAWN_MATERIAL;
    endgame_score -= n * PAWN_MATERIAL_E;
    n = pop_count(board.b_knights);
    midgame_score -= n * KNIGHT_MATERIAL;
    endgame_score -= n * KNIGHT_MATERIAL_E;
    n = pop_count(board.b_bishops);
    midgame_score -= n * BISHOP_MATERIAL;
    endgame_score -= n * BISHOP_MATERIAL_E;
    n = pop_count(board.b_rooks);
    midgame_score -= n * ROOK_MATERIAL;
    endgame_score -= n * ROOK_MATERIAL_E;
    n = pop_count(board.b_queens);
    midgame_score -= n * QUEEN_MATERIAL;
    endgame_score -= n * QUEEN_MATERIAL_E;
}

inline void pawn_structure() {
    int n = pop_count(w_pawn_attacks & board.w_pawns);
    midgame_score += n * CONNECTED_PAWN_BONUS;
    endgame_score += n * CONNECTED_PAWN_BONUS_E;
    n = pop_count(b_pawn_attacks & board.b_pawns);
    midgame_score -= n * CONNECTED_PAWN_BONUS;
    endgame_score -= n * CONNECTED_PAWN_BONUS_E;
}

inline void pawn_progression() {
    int n = pop_count(board.w_pawns & BB_RANK_5);
    midgame_score += n * RANK_5_PAWN_BONUS;
    endgame_score += n * RANK_5_PAWN_BONUS_E;
    n = pop_count(board.w_pawns & BB_RANK_6);
    midgame_score += n * RANK_6_PAWN_BONUS;
    endgame_score += n * RANK_6_PAWN_BONUS_E;
    n = pop_count(board.w_pawns & BB_RANK_7);
    midgame_score += n * RANK_7_PAWN_BONUS;
    endgame_score += n * RANK_7_PAWN_BONUS_E;

    n = pop_count(board.b_pawns & BB_RANK_5);
    midgame_score -= n * RANK_5_PAWN_BONUS;
    endgame_score -= n * RANK_5_PAWN_BONUS_E;
    n = pop_count(board.b_pawns & BB_RANK_6);
    midgame_score -= n * RANK_6_PAWN_BONUS;
    midgame_score -= n * RANK_6_PAWN_BONUS_E;
    n = pop_count(board.b_pawns & BB_RANK_7);
    midgame_score -= n * RANK_7_PAWN_BONUS;
    midgame_score -= n * RANK_7_PAWN_BONUS_E;
}

inline void doubled_pawns() {
    uint64_t mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates the number of pawns of pawns in a file - 1 and penalizes accordingly */
        int n = pop_count(board.w_pawns & mask) - 1;
        midgame_score -= n * DOUBLED_PAWN_PENALTY;
        endgame_score -= n * DOUBLED_PAWN_PENALTY_E;
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        int n = (pop_count(board.b_pawns & mask) - 1);
        midgame_score += n * DOUBLED_PAWN_PENALTY;
        endgame_score += n * DOUBLED_PAWN_PENALTY_E;
        mask <<= 1;
    }
}

inline void knight_activity() {
    /** deez knights */
    uint64_t knights = board.w_knights;
    while (knights) {
        int i = pull_lsb(&knights);
        uint64_t attacks = (BB_KNIGHT_ATTACKS[i] & ~board.w_occupied);
        while (attacks) {
            i = pull_lsb(&attacks);
            midgame_score += knight_hitboard[i];
        }
    }
    knights = reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        uint64_t attacks = (BB_KNIGHT_ATTACKS[i] & ~board.b_occupied);
        while (attacks) {
            i = pull_lsb(&attacks);
            midgame_score -= knight_hitboard[i];
        }
    }
}

inline void bishop_activity() {
    uint64_t bishops = board.w_bishops;
    while (bishops) {
    }
    bishops = board.b_bishops;
    while (bishops) {


    }
}

inline int32_t rook_activity() {

}

inline int32_t queen_activity() {

}