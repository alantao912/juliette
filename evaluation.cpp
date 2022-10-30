#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <cmath>
#include <algorithm>

/* Pointer to board object that we are currently evaluating */
extern bitboard board;

uint64_t w_pawn_attacks, b_pawn_attacks;

int32_t midgame_score, endgame_score;

inline uint64_t reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

double game_progression() {
    return 0.0;
}

int32_t evaluate() {
    midgame_score = 0;
    endgame_score = 0;
    w_pawn_attacks = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H));
    b_pawn_attacks = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A));
    material_score();
    pawn_structure();
    pawn_progression();
    doubled_pawns();
    knight_activity();
    bishop_activity();
    rook_activity();
    queen_activity();
    const double gp = game_progression();
    return (1 - 2 * (board.turn == BLACK)) * (int32_t) std::round(midgame_score * (1 - gp) + endgame_score * gp);
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
    endgame_score += n * P5_E;
    n = pop_count(board.w_pawns & BB_RANK_6);
    endgame_score += n * P6_E;
    n = pop_count(board.w_pawns & BB_RANK_7);
    endgame_score += n * P7_E;
    uint64_t pawns = board.w_pawns;
    while (pawns) {
        n = pull_lsb(&pawns);
        midgame_score += Weights::pawn_psqt[n];
    }

    uint64_t rotated_bpawns = reverse_bb(board.b_pawns);
    n = pop_count(rotated_bpawns & BB_RANK_5);
    endgame_score -= n * P5_E;
    n = pop_count(rotated_bpawns & BB_RANK_6);
    endgame_score -= n * P6_E;
    n = pop_count(rotated_bpawns & BB_RANK_7);
    endgame_score -= n * P7_E;
    pawns = board.b_rooks;
    while (pawns) {
        n = pull_lsb(&pawns);
        midgame_score -= Weights::pawn_psqt[n];
    }
}

inline void doubled_pawns() {
    uint64_t mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates the number of pawns of pawns in a file - 1 and penalizes accordingly */
        int n = std::max(pop_count(board.w_pawns & mask) - 1, 0);
        midgame_score -= n * DOUBLED_PAWN_PENALTY;
        endgame_score -= n * DOUBLED_PAWN_PENALTY_E;
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        int n = std::max((pop_count(board.b_pawns & mask) - 1), 0);
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
        midgame_score += Weights::knight_psqt[i];
    }
    knights = reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        midgame_score -= Weights::knight_psqt[i];
    }
}

inline void bishop_activity() {
    int n = pop_count(get_bishop_rays_setwise(board.w_bishops, ~board.occupied));
    midgame_score += n * HIT_BONUS;
    endgame_score += n * HIT_BONUS_E;
    n = pop_count(get_bishop_rays_setwise(board.b_bishops, ~board.occupied));
    midgame_score -= n * HIT_BONUS;
    endgame_score -= n * HIT_BONUS_E;
}

inline void rook_activity() {
    uint64_t rays = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    int n = pop_count(rays ^ board.w_rooks);
    midgame_score += n * HIT_BONUS;
    endgame_score += n * HIT_BONUS_E;
    n = pop_count(board.w_rooks & BB_RANK_7);
    midgame_score += n * ROOK_ON_7_BONUS;
    endgame_score += n * ROOK_ON_7_BONUS_E;
    n = std::max((pop_count(rays & board.w_rooks) - 1), 0);
    midgame_score += n * CONNECTED_ROOK_BONUS;
    endgame_score += n * CONNECTED_ROOK_BONUS_E;
    rays = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    n = pop_count(rays ^ board.b_rooks);
    midgame_score -= n * HIT_BONUS;
    endgame_score -= n * HIT_BONUS_E;
    n = pop_count(board.b_rooks & BB_RANK_2);
    midgame_score -= n * ROOK_ON_7_BONUS;
    endgame_score -= n * ROOK_ON_7_BONUS_E;
    n = std::max(pop_count(rays & board.b_rooks) - 1, 0);
    midgame_score -= n * CONNECTED_ROOK_BONUS;
    endgame_score -= n * CONNECTED_ROOK_BONUS_E;
}

inline void queen_activity() {
    uint64_t rays = get_queen_rays_setwise(board.w_queens, (~board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));
    int n = std::max(pop_count(rays & board.w_rooks) - 1, 0);
    midgame_score += n * QR_BATTERY;
    endgame_score += n * QR_BATTERY_E;
    n = std::max(pop_count(rays & board.w_queens) - 1, 0);
    midgame_score += n * QQ_BATTERY;
    endgame_score += n * QQ_BATTERY_E;
    n = std::max(pop_count(rays & board.w_bishops) - 1, 0);
    midgame_score += n * QB_BATTERY;
    endgame_score += n * QB_BATTERY_E;
    rays = get_queen_rays_setwise(board.b_queens, (~board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    n = std::max(pop_count(rays & board.b_rooks) - 1, 0);
    midgame_score -= n * QR_BATTERY;
    endgame_score -= n * QR_BATTERY_E;
    n = std::max(pop_count(rays & board.b_queens) - 1, 0);
    midgame_score -= n * QQ_BATTERY;
    endgame_score -= n * QQ_BATTERY_E;
    n = std::max(pop_count(rays & board.b_bishops) - 1, 0);
    midgame_score -= n * QB_BATTERY;
    endgame_score -= n * QB_BATTERY_E;
}