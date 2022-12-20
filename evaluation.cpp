#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <cmath>
#include <algorithm>

/** Global board struct */
extern bitboard board;


int32_t midgame_score, endgame_score;

inline uint64_t reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

/**
 *
 * @return floating point number from [0, 1] describing the phase of the game. 0 -> All pieces are present. 1 -> All pieces are gone
 */

double game_progression() {
    uint16_t phase = Weights::TOTAL_PHASE;
    phase -= pop_count(board.w_pawns | board.b_pawns) * Weights::PAWN_PHASE;
    phase -= pop_count(board.w_knights | board.b_knights) * Weights::KNIGHT_PHASE;
    phase -= pop_count(board.w_bishops | board.b_bishops) * Weights::BISHOP_PHASE;
    phase -= pop_count(board.w_rooks | board.b_rooks) * Weights::ROOK_PHASE;
    phase -= pop_count(board.w_queens | board.b_queens) * Weights::QUEEN_PHASE;
    return ((double) phase) / Weights::TOTAL_PHASE;
}

int32_t evaluate() {
    const double gp = game_progression();
    midgame_score = 0;
    endgame_score = 0;

    material_score();
    pawn_structure();
    doubled_pawns();
    knight_activity();
    bishop_activity();
    rook_activity();
    queen_activity();
    king_safety();
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
    int n = pop_count(get_pawn_attacks_setwise(WHITE) & board.w_pawns);
    midgame_score += n * CONNECTED_PAWN_BONUS;
    endgame_score += n * CONNECTED_PAWN_BONUS_E;
    uint64_t pawns = board.w_pawns;
    while (pawns) {
        int i = pull_lsb(&pawns);
        midgame_score += Weights::mg_pawn_psqt[i];
        endgame_score += Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(WHITE);
    int32_t pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += pawn_ctrl;

    n = pop_count(get_pawn_attacks_setwise(BLACK) & board.b_pawns);
    midgame_score -= n * CONNECTED_PAWN_BONUS;
    endgame_score -= n * CONNECTED_PAWN_BONUS_E;
    pawns = reverse_bb(board.b_pawns);
    while (pawns) {
        int i = pull_lsb(&pawns);
        midgame_score -= Weights::mg_pawn_psqt[i];
        endgame_score -= Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(BLACK);
    pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= pawn_ctrl;
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
        midgame_score += Weights::mg_knight_psqt[i];
        endgame_score += Weights::eg_knight_psqt[i];
    }
    knights = get_knight_mask_setwise(board.w_knights);
    int32_t knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += knights_ctrl / 3;

    knights = reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        midgame_score -= Weights::mg_knight_psqt[i];
        endgame_score -= Weights::eg_knight_psqt[i];
    }
    knights = get_knight_mask_setwise(board.b_knights);
    knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= knights_ctrl / 3;
}

inline void bishop_activity() {
    uint64_t data = get_bishop_rays_setwise(board.w_bishops, ~board.occupied);
    int32_t bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += bishop_ctrl / 3;
    uint64_t bishops = board.w_bishops;
    while (bishops) {
        int i = pull_lsb(&bishops);
        midgame_score += Weights::mg_bishop_psqt[i];
        endgame_score += Weights::eg_bishop_psqt[i];
    }
    data = get_bishop_rays_setwise(board.b_bishops, ~board.occupied);
    bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= bishop_ctrl / 3;
    bishops = reverse_bb(board.b_bishops);
    while (bishops) {
        int i = pull_lsb(&bishops);
        midgame_score -= Weights::mg_bishop_psqt[i];
        endgame_score -= Weights::eg_bishop_psqt[i];
    }
}

inline void rook_activity() {
    uint64_t data = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    /** Detection of connected rooks */
    int n = std::max((pop_count(data & board.w_rooks) - 1), 0);
    midgame_score += n * CONNECTED_ROOK_BONUS;
    endgame_score += n * CONNECTED_ROOK_BONUS_E;
    /** Evaluates board-control by rooks using the guard-heuristic.*/
    int32_t rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += rook_ctrl / 5;
    data = board.w_rooks;
    while (data) {
        int i = pull_lsb(&data);
        midgame_score += Weights::mg_rook_psqt[i];
        endgame_score += Weights::eg_rook_psqt[i];
    }
    /** The following repeats the same evaluation for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    midgame_score -= n * CONNECTED_ROOK_BONUS;
    endgame_score -= n * CONNECTED_ROOK_BONUS_E;

    rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= rook_ctrl / 5;
    data = reverse_bb(board.b_rooks);
    while (data) {
        int i = pull_lsb(&data);
        midgame_score -= Weights::mg_rook_psqt[i];
        endgame_score -= Weights::eg_rook_psqt[i];
    }
}

inline void queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = get_queen_rays_setwise(board.w_queens, (~board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    int n = std::max(pop_count(data & board.w_rooks) - 1, 0);
    midgame_score += n * QR_BATTERY;
    endgame_score += n * QR_BATTERY_E;

    /** Detects Queen-Bishop batteries. */
    n = std::max(pop_count(data & board.w_bishops) - 1, 0);
    midgame_score += n * QB_BATTERY;
    endgame_score += n * QB_BATTERY_E;

    /** Evaluates board-control by queens using the guard-heuristic.*/
    int32_t queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += queen_ctrl / 9;

    /** Evaluates queen placement using piece-square table */
    data = board.w_queens;
    while (data) {
        int i = pull_lsb(&data);
        midgame_score += Weights::mg_queen_psqt[i];
        endgame_score += Weights::eg_queen_psqt[i];
    }
    /** Following code duplicates the above functionality for black */
    data = get_queen_rays_setwise(board.b_queens, (~board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    midgame_score -= n * QR_BATTERY;
    endgame_score -= n * QR_BATTERY_E;
    n = std::max(pop_count(data & board.b_bishops) - 1, 0);
    midgame_score -= n * QB_BATTERY;
    endgame_score -= n * QB_BATTERY_E;
    queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= queen_ctrl / 9;
    data = reverse_bb(board.b_queens);
    while (data) {
        int i = pull_lsb(&data);
        midgame_score -= Weights::mg_queen_psqt[i];
        endgame_score -= Weights::eg_queen_psqt[i];
    }
}

void king_safety() {
    int i = get_lsb(board.w_king);
    midgame_score += Weights::mg_king_psqt[i];
    endgame_score += Weights::eg_king_psqt[i];
    i = get_lsb(reverse_bb(board.b_king));
    midgame_score -= Weights::mg_king_psqt[i];
    endgame_score -= Weights::eg_king_psqt[i];
}