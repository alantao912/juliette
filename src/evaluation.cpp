#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <cmath>
#include <algorithm>

/** Global board struct */
extern __thread bitboard board;

namespace Weights {

    const int32_t (&PAWN_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_PAWN];
    const int32_t (&KNIGHT_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_KNIGHT];
    const int32_t (&BISHOP_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_BISHOP];
    const int32_t (&ROOK_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_ROOK];
    const int32_t (&QUEEN_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_QUEEN];
    const int32_t (&KING_PSQT)[64] = Weights::PSQTs[piece_t::BLACK_KING];

    namespace Endgame {
        const int32_t (&PAWN_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_PAWN];
        const int32_t (&KNIGHT_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_KNIGHT];
        const int32_t (&BISHOP_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_BISHOP];
        const int32_t (&ROOK_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_ROOK];
        const int32_t (&QUEEN_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_QUEEN];
        const int32_t (&KING_PSQT)[64] = Weights::Endgame::PSQTs[piece_t::BLACK_KING];
    }
}

void Evaluation::reset() {
    midgame_score = 0;
    endgame_score = 0;
    w_king_vulnerabilities = compute_king_vulnerabilities(board.w_king, board.w_pawns);
    b_king_vulnerabilities = compute_king_vulnerabilities(board.b_king, board.b_pawns);
    progression = compute_progression();
}

int32_t Evaluation::compute_score() const {
    return (1 - 2 * (board.turn == BLACK)) *
           (int32_t) std::round(midgame_score * (1 - progression) + endgame_score * progression);
}

/**
 * Computes a bitboard of vulnerable squares around the king. Opponent gets bonus of pieces hit these squares.
 * @param king bitboard with king
 * @param pawns bitboard with pawns of the same color as the king
 * @return a bitboard of vulnerable squares around the king.
 */

uint64_t Evaluation::compute_king_vulnerabilities(uint64_t king, uint64_t pawns) {
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

double Evaluation::compute_progression() {
    uint16_t phase = Weights::TOTAL_PHASE;
    phase -= pop_count(board.w_pawns | board.b_pawns) * Weights::PAWN_PHASE;
    phase -= pop_count(board.w_knights | board.b_knights) * Weights::KNIGHT_PHASE;
    phase -= pop_count(board.w_bishops | board.b_bishops) * Weights::BISHOP_PHASE;
    phase -= pop_count(board.w_rooks | board.b_rooks) * Weights::ROOK_PHASE;
    phase -= pop_count(board.w_queens | board.b_queens) * Weights::QUEEN_PHASE;
    return ((double) phase) / Weights::TOTAL_PHASE;
}

int32_t Evaluation::evaluate() {
    reset();
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
    return compute_score();
}

void Evaluation::material_score() {
    /** White Material Score */
    int n = pop_count(board.w_pawns);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    n = pop_count(board.w_knights);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    n = pop_count(board.w_bishops);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    n = pop_count(board.w_rooks);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    n = pop_count(board.w_queens);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];

    /** Black material score */
    n = pop_count(board.b_pawns);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    n = pop_count(board.b_knights);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    n = pop_count(board.b_bishops);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    n = pop_count(board.b_rooks);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    n = pop_count(board.b_queens);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
}

void Evaluation::pawn_structure() {
    int n = pop_count(get_pawn_attacks_setwise(WHITE) & board.w_pawns);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::PAWN_CHAIN];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::PAWN_CHAIN];
    uint64_t pawns = board.w_pawns;
    while (pawns) {
        int i = pull_lsb(&pawns);
        midgame_score += Weights::PAWN_PSQT[i];
        endgame_score += Weights::Endgame::PAWN_PSQT[i];
    }
    pawns = get_pawn_attacks_setwise(WHITE);

    n = pop_count(pawns & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    int32_t pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += pawn_ctrl;

    n = pop_count(get_pawn_attacks_setwise(BLACK) & board.b_pawns);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::PAWN_CHAIN];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::PAWN_CHAIN];
    pawns = _get_reverse_bb(board.b_pawns);
    while (pawns) {
        int i = pull_lsb(&pawns);
        midgame_score -= Weights::PAWN_PSQT[i];
        endgame_score -= Weights::Endgame::PAWN_PSQT[i];
    }
    pawns = get_pawn_attacks_setwise(BLACK);

    n = pop_count(pawns & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    pawn_ctrl = 0;
    while (pawns) {
        int i = pull_lsb(&pawns);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= pawn_ctrl;
}

void Evaluation::doubled_pawns() {
    uint64_t mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates the number of pawns of pawns in a file - 1 and penalizes accordingly */
        int n = std::max(pop_count(board.w_pawns & mask) - 1, 0);
        midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::DOUBLED_PAWNS];
        endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::DOUBLED_PAWNS];
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        int n = std::max((pop_count(board.b_pawns & mask) - 1), 0);
        midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::DOUBLED_PAWNS];
        endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::DOUBLED_PAWNS];
        mask <<= 1;
    }
}

void Evaluation::knight_activity() {
    uint64_t knights = board.w_knights;
    while (knights) {
        int i = pull_lsb(&knights);
        midgame_score += Weights::KNIGHT_PSQT[i];
        endgame_score += Weights::Endgame::KNIGHT_PSQT[i];
    }
    knights = get_knight_mask_setwise(board.w_knights);

    int n = pop_count(knights & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    int32_t knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += knights_ctrl / 3;

    knights = _get_reverse_bb(board.b_knights);
    while (knights) {
        int i = pull_lsb(&knights);
        midgame_score -= Weights::KNIGHT_PSQT[i];
        endgame_score -= Weights::Endgame::KNIGHT_PSQT[i];
    }
    knights = get_knight_mask_setwise(board.b_knights);

    n = pop_count(knights & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= knights_ctrl / 3;
}

void Evaluation::bishop_activity() {
    uint64_t data = get_bishop_rays_setwise(board.w_bishops, ~board.occupied);

    int n = pop_count(data & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    int32_t bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += bishop_ctrl / 3;
    uint64_t bishops = board.w_bishops;
    while (bishops) {
        int i = pull_lsb(&bishops);
        midgame_score += Weights::BISHOP_PSQT[i];
        endgame_score += Weights::Endgame::BISHOP_PSQT[i];
    }
    data = get_bishop_rays_setwise(board.b_bishops, ~board.occupied);

    n = pop_count(data & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    bishop_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        bishop_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= bishop_ctrl / 3;
    bishops = _get_reverse_bb(board.b_bishops);
    while (bishops) {
        int i = pull_lsb(&bishops);
        midgame_score -= Weights::BISHOP_PSQT[i];
        endgame_score -= Weights::Endgame::BISHOP_PSQT[i];
    }
}

void Evaluation::rook_activity() {
    uint64_t data = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    /** Detection of connected rooks */
    int n = std::max((pop_count(data & board.w_rooks) - 1), 0);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::CONNECTED_ROOKS];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::CONNECTED_ROOKS];

    n = pop_count(data & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

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
        midgame_score += Weights::ROOK_PSQT[i];
        endgame_score += Weights::Endgame::ROOK_PSQT[i];
    }
    /** The following repeats the same score for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::CONNECTED_ROOKS];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::CONNECTED_ROOKS];

    n = pop_count(data & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= rook_ctrl / 5;
    data = _get_reverse_bb(board.b_rooks);
    while (data) {
        int i = pull_lsb(&data);
        midgame_score -= Weights::ROOK_PSQT[i];
        endgame_score -= Weights::Endgame::ROOK_PSQT[i];
    }
}

void Evaluation::queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = get_queen_rays_setwise(board.w_queens,
                                           (~board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    int n = std::max(pop_count(data & board.w_rooks) - 1, 0);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::QUEEN_ROOK];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::QUEEN_ROOK];

    /** Detects Queen-Bishop batteries. */
    n = std::max(pop_count(data & board.w_bishops) - 1, 0);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::QUEEN_BISHOP];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::QUEEN_BISHOP];

    n = pop_count(data & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

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
        midgame_score += Weights::QUEEN_PSQT[i];
        endgame_score += Weights::Endgame::QUEEN_PSQT[i];
    }
    /** Following code duplicates the above functionality for black */
    data = get_queen_rays_setwise(board.b_queens, (~board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    n = std::max(pop_count(data & board.b_rooks) - 1, 0);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::QUEEN_ROOK];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::QUEEN_ROOK];

    n = std::max(pop_count(data & board.b_bishops) - 1, 0);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::QUEEN_BISHOP];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::QUEEN_BISHOP];

    n = pop_count(data & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= queen_ctrl / 9;
    data = _get_reverse_bb(board.b_queens);
    while (data) {
        int i = pull_lsb(&data);
        midgame_score -= Weights::QUEEN_PSQT[i];
        endgame_score -= Weights::Endgame::QUEEN_PSQT[i];
    }
}

void Evaluation::king_safety() {
    int i = get_lsb(board.w_king);
    midgame_score += Weights::KING_PSQT[i];
    endgame_score += Weights::Endgame::KING_PSQT[i];


    i = get_lsb(_get_reverse_bb(board.b_king));
    midgame_score -= Weights::KING_PSQT[i];
    endgame_score -= Weights::Endgame::KING_PSQT[i];
}

void Evaluation::king_mobility() {
    int w_file = board.w_king_square % 8, w_rank = board.w_king_square / 8;

    w_file = std::min(w_file, 7 - w_file);
    w_rank = std::min(w_rank, 7 - w_rank);
    endgame_score += ((w_file * w_file) + (w_rank * w_rank)) * Weights::CENTRALIZED_KING;

    int b_file = board.b_king_square % 8, b_rank = board.b_king_square / 8;
    b_file = std::min(b_file, 7 - b_file);
    b_rank = std::min(b_rank, 7 - b_rank);
    endgame_score -= ((b_file * b_file) + (b_rank * b_rank)) * Weights::CENTRALIZED_KING;

    // TODO: Implement opposition detection. Direct opposition, Distant opposition, Diagonal Opposition

}

/**
 * Determines the number of unopposed pawns, or "passed" pawns. Passed pawns on the side of the board are worth less than passed pawns
 * toward the middle of the board.
 */

void Evaluation::passed_pawns() {
    // TODO: Implement
}