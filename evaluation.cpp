#include "movegen.h"
#include "weights.h"
#include "evaluation.h"

#include <cmath>
#include <cstring>
#include <algorithm>

/** Global board struct */
extern bitboard board;
static struct eval_stats stats;

void eval_stats::reset() {
    midgame_score = 0;
    endgame_score = 0;
    w_king_vulnerabilities = compute_king_vulnerabilities(board.w_king, board.w_pawns);
    b_king_vulnerabilities = compute_king_vulnerabilities(board.b_king, board.b_pawns);
    progression = compute_progression();
}

int32_t eval_stats::compute_score() {
    return (1 - 2 * (board.turn == BLACK)) * (int32_t) std::round(midgame_score * (1 - progression) + endgame_score * progression);
}

/**
 * Computes a bitboard of vulnerable squares around the king. Opponent gets bonus of pieces hit these squares.
 * @param king bitboard with king
 * @param pawns bitboard with pawns of the same color as the king
 * @return a bitboard of vulnerable squares around the king.
 */

uint64_t eval_stats::compute_king_vulnerabilities(uint64_t king, uint64_t pawns) {
    /** Highlights the squares around the king */
    uint64_t king_occ = 0, temp  = (king << 1) & ~BB_FILE_A;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~pawns;
    temp |= ((temp << 1) & ~BB_FILE_A);
    temp &= ~pawns;
    king_occ |= temp;
    temp = (king >> 1) & ~BB_FILE_H;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~pawns;
    temp |= ((temp >> 1) & ~BB_FILE_H);
    temp &= ~pawns;
    king_occ |= temp;

    temp = king >> 8;
    temp |= ((temp << 1) & ~BB_FILE_A) | ((temp >> 1) & ~BB_FILE_H);
    temp &= ~pawns;
    king_occ |= temp;
    temp >>= 8;
    temp &= ~pawns;
    king_occ |= temp;

    temp = king << 8;
    temp |= ((temp << 1) & ~BB_FILE_A) | ((temp >> 1) & ~BB_FILE_H);
    temp &= ~pawns;
    king_occ |= temp;
    temp <<= 8;
    temp &= ~pawns;
    king_occ |= temp;
    return king_occ;
}

/**
 * Computes and returns a floating point number representing the game phase. [0, 1] Opening -> Endgame.
 * @return floating point number from [0, 1] describing the phase of the game. 0 -> All pieces are present. 1 -> All pieces are gone
 */

double eval_stats::compute_progression() {
    uint16_t phase = Weights::TOTAL_PHASE;
    phase -= pop_count(board.w_pawns | board.b_pawns) * Weights::PAWN_PHASE;
    phase -= pop_count(board.w_knights | board.b_knights) * Weights::KNIGHT_PHASE;
    phase -= pop_count(board.w_bishops | board.b_bishops) * Weights::BISHOP_PHASE;
    phase -= pop_count(board.w_rooks | board.b_rooks) * Weights::ROOK_PHASE;
    phase -= pop_count(board.w_queens | board.b_queens) * Weights::QUEEN_PHASE;
    return ((double) phase) / Weights::TOTAL_PHASE;
}

inline uint64_t reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

int32_t evaluate() {
    stats.reset();
    material_score();
    pawn_structure();
    doubled_pawns();
    knight_activity();
    bishop_activity();
    rook_activity();
    queen_activity();
    king_safety();
    king_mobility();
    passed_pawns();
    return stats.compute_score();
}

inline void material_score() {
    /** White Material Score */
    int n = pop_count(board.w_pawns);
    stats.midgame_score += n * PAWN_MATERIAL;
    stats.endgame_score += n * PAWN_MATERIAL_E;
    n = pop_count(board.w_knights);
    stats.midgame_score += n * KNIGHT_MATERIAL;
    stats.endgame_score += n * KNIGHT_MATERIAL_E;
    n = pop_count(board.w_bishops);
    stats.midgame_score += n * BISHOP_MATERIAL;
    stats.endgame_score += n * BISHOP_MATERIAL_E;
    n = pop_count(board.w_rooks);
    stats.midgame_score += n * ROOK_MATERIAL;
    stats.endgame_score += n * ROOK_MATERIAL_E;
    n = pop_count(board.w_queens);
    stats.midgame_score += n * QUEEN_MATERIAL;
    stats.endgame_score += n * QUEEN_MATERIAL_E;

    /** Black material score */
    n = pop_count(board.b_pawns);
    stats.midgame_score -= n * PAWN_MATERIAL;
    stats.endgame_score -= n * PAWN_MATERIAL_E;
    n = pop_count(board.b_knights);
    stats.midgame_score -= n * KNIGHT_MATERIAL;
    stats.endgame_score -= n * KNIGHT_MATERIAL_E;
    n = pop_count(board.b_bishops);
    stats.midgame_score -= n * BISHOP_MATERIAL;
    stats.endgame_score -= n * BISHOP_MATERIAL_E;
    n = pop_count(board.b_rooks);
    stats.midgame_score -= n * ROOK_MATERIAL;
    stats.endgame_score -= n * ROOK_MATERIAL_E;
    n = pop_count(board.b_queens);
    stats.midgame_score -= n * QUEEN_MATERIAL;
    stats.endgame_score -= n * QUEEN_MATERIAL_E;
}

inline void pawn_structure() {
    int n = pop_count(get_pawn_attacks_setwise(WHITE) & board.w_pawns);
    stats.midgame_score += n * CONNECTED_PAWN_BONUS;
    stats.endgame_score += n * CONNECTED_PAWN_BONUS_E;
    uint64_t pawns = board.w_pawns;
    while (pawns) {
        int i = pull_lsb(&pawns);
        stats.midgame_score += Weights::mg_pawn_psqt[i];
        stats.endgame_score += Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(WHITE);

    n = pop_count(pawns & stats.b_king_vulnerabilities);
    stats.midgame_score += n * KING_THREAT;
    stats.endgame_score += n * KING_THREAT_E;

    int32_t pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += pawn_ctrl;

    n = pop_count(get_pawn_attacks_setwise(BLACK) & board.b_pawns);
    stats.midgame_score -= n * CONNECTED_PAWN_BONUS;
    stats.endgame_score -= n * CONNECTED_PAWN_BONUS_E;
    pawns = reverse_bb(board.b_pawns);
    while (pawns) {
        int i = pull_lsb(&pawns);
        stats.midgame_score -= Weights::mg_pawn_psqt[i];
        stats.endgame_score -= Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(BLACK);

    n = pop_count(pawns & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * KING_THREAT;
    stats.endgame_score -= n * KING_THREAT_E;

    pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= pawn_ctrl;
}

inline void doubled_pawns() {
    uint64_t mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates the number of pawns of pawns in a file - 1 and penalizes accordingly */
        int n = std::max(pop_count(board.w_pawns & mask) - 1, 0);
        stats.midgame_score -= n * DOUBLED_PAWN_PENALTY;
        stats.endgame_score -= n * DOUBLED_PAWN_PENALTY_E;
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        int n = std::max((pop_count(board.b_pawns & mask) - 1), 0);
        stats.midgame_score += n * DOUBLED_PAWN_PENALTY;
        stats.endgame_score += n * DOUBLED_PAWN_PENALTY_E;
        mask <<= 1;
    }
}

inline void knight_activity() {
    /** deez knights */
    uint64_t knights = board.w_knights;
    while (knights) {
        int i = pull_lsb(&knights);
        stats.midgame_score += Weights::mg_knight_psqt[i];
        stats.endgame_score += Weights::eg_knight_psqt[i];
    }
    knights = get_knight_mask_setwise(board.w_knights);

    int n = pop_count(knights & stats.b_king_vulnerabilities);
    stats.midgame_score += n * KING_THREAT;
    stats.endgame_score += n * KING_THREAT_E;

    int32_t knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += knights_ctrl / 3;

    knights = reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        stats.midgame_score -= Weights::mg_knight_psqt[i];
        stats.endgame_score -= Weights::eg_knight_psqt[i];
    }
    knights = get_knight_mask_setwise(board.b_knights);

    n = pop_count(knights & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * KING_THREAT;
    stats.endgame_score -= n * KING_THREAT_E;

    knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= knights_ctrl / 3;
}

inline void bishop_activity() {
    uint64_t data = get_bishop_rays_setwise(board.w_bishops, ~board.occupied);

    int n = pop_count(data & stats.b_king_vulnerabilities);
    stats.midgame_score += n * KING_THREAT;
    stats.endgame_score += n * KING_THREAT_E;

    int32_t bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += bishop_ctrl / 3;
    uint64_t bishops = board.w_bishops;
    while (bishops) {
        int i = pull_lsb(&bishops);
        stats.midgame_score += Weights::mg_bishop_psqt[i];
        stats.endgame_score += Weights::eg_bishop_psqt[i];
    }
    data = get_bishop_rays_setwise(board.b_bishops, ~board.occupied);

    n = pop_count(data & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * KING_THREAT;
    stats.endgame_score -= n * KING_THREAT_E;

    bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= bishop_ctrl / 3;
    bishops = reverse_bb(board.b_bishops);
    while (bishops) {
        int i = pull_lsb(&bishops);
        stats.midgame_score -= Weights::mg_bishop_psqt[i];
        stats.endgame_score -= Weights::eg_bishop_psqt[i];
    }
}

inline void rook_activity() {
    uint64_t data = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    /** Detection of connected rooks */
    int n = std::max((pop_count(data & board.w_rooks) - 1), 0);
    stats.midgame_score += n * CONNECTED_ROOK_BONUS;
    stats.endgame_score += n * CONNECTED_ROOK_BONUS_E;

    n = pop_count(data & stats.b_king_vulnerabilities);
    stats.midgame_score += n * KING_THREAT;
    stats.endgame_score += n * KING_THREAT_E;

    /** Evaluates board-control by rooks using the guard-heuristic.*/
    int32_t rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += rook_ctrl / 5;
    data = board.w_rooks;
    while (data) {
        int i = pull_lsb(&data);
        stats.midgame_score += Weights::mg_rook_psqt[i];
        stats.endgame_score += Weights::eg_rook_psqt[i];
    }
    /** The following repeats the same evaluation for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    stats.midgame_score -= n * CONNECTED_ROOK_BONUS;
    stats.endgame_score -= n * CONNECTED_ROOK_BONUS_E;

    n = pop_count(data & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * KING_THREAT;
    stats.endgame_score -= n * KING_THREAT_E;

    rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= rook_ctrl / 5;
    data = reverse_bb(board.b_rooks);
    while (data) {
        int i = pull_lsb(&data);
        stats.midgame_score -= Weights::mg_rook_psqt[i];
        stats.endgame_score -= Weights::eg_rook_psqt[i];
    }
}

inline void queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = get_queen_rays_setwise(board.w_queens, (~board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    int n = std::max(pop_count(data & board.w_rooks) - 1, 0);
    stats.midgame_score += n * QR_BATTERY;
    stats.endgame_score += n * QR_BATTERY_E;

    /** Detects Queen-Bishop batteries. */
    n = std::max(pop_count(data & board.w_bishops) - 1, 0);
    stats.midgame_score += n * QB_BATTERY;
    stats.endgame_score += n * QB_BATTERY_E;

    n = pop_count(data & stats.b_king_vulnerabilities);
    stats.midgame_score += n * KING_THREAT;
    stats.endgame_score += n * KING_THREAT_E;

    /** Evaluates board-control by queens using the guard-heuristic.*/
    int32_t queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += queen_ctrl / 9;

    /** Evaluates queen placement using piece-square table */
    data = board.w_queens;
    while (data) {
        int i = pull_lsb(&data);
        stats.midgame_score += Weights::mg_queen_psqt[i];
        stats.endgame_score += Weights::eg_queen_psqt[i];
    }
    /** Following code duplicates the above functionality for black */
    data = get_queen_rays_setwise(board.b_queens, (~board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    stats.midgame_score -= n * QR_BATTERY;
    stats.endgame_score -= n * QR_BATTERY_E;

    n = std::max(pop_count(data & board.b_bishops) - 1, 0);
    stats.midgame_score -= n * QB_BATTERY;
    stats.endgame_score -= n * QB_BATTERY_E;

    n = pop_count(data & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * KING_THREAT;
    stats.endgame_score -= n * KING_THREAT_E;

    queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= queen_ctrl / 9;
    data = reverse_bb(board.b_queens);
    while (data) {
        int i = pull_lsb(&data);
        stats.midgame_score -= Weights::mg_queen_psqt[i];
        stats.endgame_score -= Weights::eg_queen_psqt[i];
    }
}

void king_safety() {
    int i = get_lsb(board.w_king);
    stats.midgame_score += Weights::mg_king_psqt[i];
    stats.endgame_score += Weights::eg_king_psqt[i];


    i = get_lsb(reverse_bb(board.b_king));
    stats.midgame_score -= Weights::mg_king_psqt[i];
    stats.endgame_score -= Weights::eg_king_psqt[i];
}

void king_mobility() {
    int w_file = board.w_king_square % 8, w_rank = board.w_king_square / 8;

    w_file = std::min(w_file, 7 - w_file);
    w_rank = std::min(w_rank, 7 - w_rank);
    stats.endgame_score += ((w_file * w_file) + (w_rank * w_rank)) * CENTRALIZED_KING;

    int b_file = board.b_king_square % 8, b_rank = board.b_king_square / 8;
    b_file = std::min(b_file, 7 - b_file);
    b_rank = std::min(b_rank, 7 - b_rank);
    stats.endgame_score -= ((b_file * b_file) + (b_rank * b_rank)) * CENTRALIZED_KING;

    // TODO: Implement opposition detection. Direct opposition, Distant opposition, Diagonal Opposition

}

/**
 * Determines the number of unopposed pawns, or "passed" pawns. Passed pawns on the side of the baord are worth less than passed pawns
 * toward the middle of the board.
 */

void passed_pawns() {
    // TODO: Implement
}