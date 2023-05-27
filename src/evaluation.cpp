#include "movegen.h"
#include "weights.h"
#include "evaluation.h"

#include <cmath>
#include <algorithm>

/** Global board struct */
extern __thread bitboard board;
static __thread struct eval_stats stats;

void eval_stats::reset() {
    midgame_score = 0;
    endgame_score = 0;
    w_king_vulnerabilities = compute_king_vulnerabilities(board.w_king, board.w_pawns);
    b_king_vulnerabilities = compute_king_vulnerabilities(board.b_king, board.b_pawns);
    progression = compute_progression();
}

int32_t eval_stats::compute_score() {
    return (1 - 2 * (board.turn == BLACK)) *
           (int32_t) std::round(midgame_score * (1 - progression) + endgame_score * progression);
}

/**
 * Computes a bitboard of vulnerable squares around the king. Opponent gets bonus of pieces hit these squares.
 * @param king bitboard with king
 * @param pawns bitboard with pawns of the same color as the king
 * @return a bitboard of vulnerable squares around the king.
 */

uint64_t eval_stats::compute_king_vulnerabilities(uint64_t king, uint64_t pawns) {
    /** Highlights the squares around the king */
    uint64_t king_occ = 0, temp = (king << 1) & ~BB_FILE_A;
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
    stats.midgame_score += n * Weights::PAWN_MATERIAL;
    stats.endgame_score += n * Weights::PAWN_MATERIAL_EG;
    n = pop_count(board.w_knights);
    stats.midgame_score += n * Weights::KNIGHT_MATERIAL;
    stats.endgame_score += n * Weights::KNIGHT_MATERIAL_EG;
    n = pop_count(board.w_bishops);
    stats.midgame_score += n * Weights::BISHOP_MATERIAL;
    stats.endgame_score += n * Weights::BISHOP_MATERIAL_EG;
    n = pop_count(board.w_rooks);
    stats.midgame_score += n * Weights::ROOK_MATERIAL;
    stats.endgame_score += n * Weights::ROOK_MATERIAL_EG;
    n = pop_count(board.w_queens);
    stats.midgame_score += n * Weights::QUEEN_MATERIAL;
    stats.endgame_score += n * Weights::QUEEN_MATERIAL_EG;

    /** Black material score */
    n = pop_count(board.b_pawns);
    stats.midgame_score -= n * Weights::PAWN_MATERIAL;
    stats.endgame_score -= n * Weights::PAWN_MATERIAL_EG;
    n = pop_count(board.b_knights);
    stats.midgame_score -= n * Weights::KNIGHT_MATERIAL;
    stats.endgame_score -= n * Weights::KNIGHT_MATERIAL_EG;
    n = pop_count(board.b_bishops);
    stats.midgame_score -= n * Weights::BISHOP_MATERIAL;
    stats.endgame_score -= n * Weights::BISHOP_MATERIAL_EG;
    n = pop_count(board.b_rooks);
    stats.midgame_score -= n * Weights::ROOK_MATERIAL;
    stats.endgame_score -= n * Weights::ROOK_MATERIAL_EG;
    n = pop_count(board.b_queens);
    stats.midgame_score -= n * Weights::QUEEN_MATERIAL;
    stats.endgame_score -= n * Weights::QUEEN_MATERIAL_EG;
}

inline void pawn_structure() {
    int n = pop_count(get_pawn_attacks_setwise(WHITE) & board.w_pawns);
    stats.midgame_score += n * Weights::CONNECTED_PAWNS;
    stats.endgame_score += n * Weights::CONNECTED_PAWNS_EG;
    uint64_t pawns = board.w_pawns;
    while (pawns) {
        int i = pull_lsb(&pawns);
        stats.midgame_score += Weights::mg_pawn_psqt[i];
        stats.endgame_score += Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(WHITE);

    n = pop_count(pawns & stats.b_king_vulnerabilities);
    stats.midgame_score += n * Weights::KING_THREAT;
    stats.endgame_score += n * Weights::KING_THREAT_EG;

    int32_t pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += pawn_ctrl;

    n = pop_count(get_pawn_attacks_setwise(BLACK) & board.b_pawns);
    stats.midgame_score -= n * Weights::CONNECTED_PAWNS;
    stats.endgame_score -= n * Weights::CONNECTED_PAWNS_EG;
    pawns = _get_reverse_bb(board.b_pawns);
    while (pawns) {
        int i = pull_lsb(&pawns);
        stats.midgame_score -= Weights::mg_pawn_psqt[i];
        stats.endgame_score -= Weights::eg_pawn_psqt[i];
    }
    pawns = get_pawn_attacks_setwise(BLACK);

    n = pop_count(pawns & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * Weights::KING_THREAT;
    stats.endgame_score -= n * Weights::KING_THREAT_EG;

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
        stats.midgame_score -= n * Weights::DOUBLED_PAWN_PENALTY;
        stats.endgame_score -= n * Weights::DOUBLED_PAWN_PENALTY_EG;
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        int n = std::max((pop_count(board.b_pawns & mask) - 1), 0);
        stats.midgame_score += n * Weights::DOUBLED_PAWN_PENALTY;
        stats.endgame_score += n * Weights::DOUBLED_PAWN_PENALTY_EG;
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
    stats.midgame_score += n * Weights::KING_THREAT;
    stats.endgame_score += n * Weights::KING_THREAT_EG;

    int32_t knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score += knights_ctrl / 3;

    knights = _get_reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        stats.midgame_score -= Weights::mg_knight_psqt[i];
        stats.endgame_score -= Weights::eg_knight_psqt[i];
    }
    knights = get_knight_mask_setwise(board.b_knights);

    n = pop_count(knights & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * Weights::KING_THREAT;
    stats.endgame_score -= n * Weights::KING_THREAT_EG;

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
    stats.midgame_score += n * Weights::KING_THREAT;
    stats.endgame_score += n * Weights::KING_THREAT_EG;

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
    stats.midgame_score -= n * Weights::KING_THREAT;
    stats.endgame_score -= n * Weights::KING_THREAT_EG;

    bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= bishop_ctrl / 3;
    bishops = _get_reverse_bb(board.b_bishops);
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
    stats.midgame_score += n * Weights::CONNECTED_ROOK_BONUS;
    stats.endgame_score += n * Weights::CONNECTED_ROOK_BONUS_EG;

    n = pop_count(data & stats.b_king_vulnerabilities);
    stats.midgame_score += n * Weights::KING_THREAT;
    stats.endgame_score += n * Weights::KING_THREAT_EG;

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
    /** The following repeats the same score for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    stats.midgame_score -= n * Weights::CONNECTED_ROOK_BONUS;
    stats.endgame_score -= n * Weights::CONNECTED_ROOK_BONUS_EG;

    n = pop_count(data & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * Weights::KING_THREAT;
    stats.endgame_score -= n * Weights::KING_THREAT_EG;

    rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= rook_ctrl / 5;
    data = _get_reverse_bb(board.b_rooks);
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
    uint64_t data = get_queen_rays_setwise(board.w_queens,
                                           ~(board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    int n = std::max(pop_count(data & board.w_rooks) - 1, 0);
    stats.midgame_score += n * Weights::QR_BATTERY;
    stats.endgame_score += n * Weights::QR_BATTERY_EG;

    /** Detects Queen-Bishop batteries. */
    n = std::max(pop_count(data & board.w_bishops) - 1, 0);
    stats.midgame_score += n * Weights::QB_BATTERY;
    stats.endgame_score += n * Weights::QB_BATTERY_EG;

    n = pop_count(data & stats.b_king_vulnerabilities);
    stats.midgame_score += n * Weights::KING_THREAT;
    stats.endgame_score += n * Weights::KING_THREAT_EG;

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
    data = get_queen_rays_setwise(board.b_queens, ~(board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    stats.midgame_score -= n * Weights::QR_BATTERY;
    stats.endgame_score -= n * Weights::QR_BATTERY_EG;

    n = std::max(pop_count(data & board.b_bishops) - 1, 0);
    stats.midgame_score -= n * Weights::QB_BATTERY;
    stats.endgame_score -= n * Weights::QB_BATTERY_EG;

    n = pop_count(data & stats.w_king_vulnerabilities);
    stats.midgame_score -= n * Weights::KING_THREAT;
    stats.endgame_score -= n * Weights::KING_THREAT_EG;

    queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    stats.midgame_score -= queen_ctrl / 9;
    data = _get_reverse_bb(board.b_queens);
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


    i = get_lsb(_get_reverse_bb(board.b_king));
    stats.midgame_score -= Weights::mg_king_psqt[i];
    stats.endgame_score -= Weights::eg_king_psqt[i];
}

void king_mobility() {
    int w_file = board.w_king_square % 8, w_rank = board.w_king_square / 8;

    w_file = std::min(w_file, 7 - w_file);
    w_rank = std::min(w_rank, 7 - w_rank);
    stats.endgame_score += ((w_file * w_file) + (w_rank * w_rank)) * Weights::CENTRALIZED_KING;

    int b_file = board.b_king_square % 8, b_rank = board.b_king_square / 8;
    b_file = std::min(b_file, 7 - b_file);
    b_rank = std::min(b_rank, 7 - b_rank);
    stats.endgame_score -= ((b_file * b_file) + (b_rank * b_rank)) * Weights::CENTRALIZED_KING;

    // TODO: Implement opposition detection. Direct opposition, Distant opposition, Diagonal Opposition

}

/**
 * Determines the number of unopposed pawns, or "passed" pawns. Passed pawns on the side of the baord are worth less than passed pawns
 * toward the middle of the board.
 */

void passed_pawns() {
    // TODO: Implement
}