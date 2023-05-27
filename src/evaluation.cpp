#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

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
    compute_n_open_files();
    compute_space_bonus_w();
    compute_space_bonus_b();
    progression = compute_progression();
}

int32_t Evaluation::compute_score() const {
    return (1 - 2 * (board.turn == BLACK)) *
           (int32_t) std::round(midgame_score * (1 - progression) + endgame_score * progression);
}

/**
 * Computes a bitboard of vulnerable squares around the king. Opponent gets bonus of pieces hit these squares.
 * @param king bitboard with king
 * @param barriers bitboard with barriers that protect the king (could be pieces, pawns, or nothing)
 * @return a bitboard of vulnerable squares around the king.
 */

uint64_t Evaluation::compute_king_vulnerabilities(uint64_t king, uint64_t barriers) {
    /** Highlights the squares around the king */
    uint64_t king_occ = 0, temp = (king << 1) & ~BB_FILE_A;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~barriers;
    temp |= ((temp << 1) & ~BB_FILE_A);
    temp &= ~barriers;
    king_occ |= temp;
    temp = (king >> 1) & ~BB_FILE_H;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~barriers;
    temp |= ((temp >> 1) & ~BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;

    temp = king >> 8;
    temp |= ((temp << 1) & ~BB_FILE_A) | ((temp >> 1) & ~BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;
    temp >>= 8;
    temp &= ~barriers;
    king_occ |= temp;

    temp = king << 8;
    temp |= ((temp << 1) & ~BB_FILE_A) | ((temp >> 1) & ~BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;
    temp <<= 8;
    temp &= ~barriers;
    king_occ |= temp;
    return king_occ;
}

uint64_t Evaluation::compute_pawn_frontspans_w(uint64_t pawns_bb, uint64_t occupied_bb) {
    uint64_t frontspans = 0ULL;
    do {
        pawns_bb <<= 8;
        pawns_bb ^= (pawns_bb & occupied_bb);
        frontspans |= pawns_bb;
    } while (pawns_bb);
    return frontspans;
}

uint64_t Evaluation::compute_pawn_frontspans_b(uint64_t pawns_bb, uint64_t occupied_bb) {
    uint64_t frontspans = 0ULL;
    do {
        pawns_bb >>= 8;
        pawns_bb ^= (pawns_bb & occupied_bb);
        frontspans |= pawns_bb;
    } while (pawns_bb);
    return frontspans;
}

uint64_t Evaluation::compute_pawn_backspans_w(uint64_t pawns_bb, uint64_t occupied_bb) {
    return compute_pawn_frontspans_b(pawns_bb, occupied_bb);
}

uint64_t Evaluation::compute_pawn_backspans_b(uint64_t pawns_bb, uint64_t occupied_bb) {
    return compute_pawn_frontspans_w(pawns_bb, occupied_bb);
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

void Evaluation::compute_space_bonus_w() {
    int n_pieces = pop_count(board.w_rooks) + pop_count(board.w_knights)
                   + pop_count(board.w_bishops) + pop_count(board.w_queens);
    w_space_bonus = n_pieces - n_open_files;
}

void Evaluation::compute_space_bonus_b() {
    int n_pieces = pop_count(board.b_rooks) + pop_count(board.b_knights) +
                   pop_count(board.b_bishops) + pop_count(board.b_queens);
    b_space_bonus = n_pieces - n_open_files;
}

void Evaluation::compute_n_open_files() {
    uint64_t file_mask = BB_FILE_A;
    int32_t n = 0;
    for (int i = 0; i < 8; ++i) {
        n += ((file_mask & board.occupied) == 0);
        file_mask <<= 1;
    }
    n_open_files = n;
}

int32_t Evaluation::evaluate() {
    reset();
    material_score();
    accumulate_psqts();

    pawn_structure();
    passed_pawns();
    doubled_pawns();
    backward_pawns();
    knight_activity();
    bishop_activity();
    rook_activity();
    queen_activity();
    king_placement();
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
    uint64_t pawn_attacks = get_pawn_attacks_setwise(WHITE);
    accumulate_characteristic_w(pop_count(pawn_attacks & board.w_pawns),
                                Evaluation::characteristic_t::PAWN_CHAIN);

    int n = pop_count(pawn_attacks & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    int32_t pawn_ctrl = 0;
    while (pawn_attacks) {
        int i = pull_lsb(&pawn_attacks);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += pawn_ctrl;

    pawn_attacks = get_pawn_attacks_setwise(BLACK);
    accumulate_characteristic_b(pop_count(pawn_attacks & board.b_pawns), Evaluation::characteristic_t::PAWN_CHAIN);

    n = pop_count(pawn_attacks & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    pawn_ctrl = 0;
    while (pawn_attacks) {
        int i = pull_lsb(&pawn_attacks);
        pawn_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= pawn_ctrl;
}

void Evaluation::doubled_pawns() {
    uint64_t mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates (number of pawns in a file - 1) and penalizes accordingly */
        accumulate_characteristic_w(std::max(pop_count(board.w_pawns & mask) - 1, 0),
                                    Evaluation::characteristic_t::DOUBLED_PAWNS);
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        accumulate_characteristic_b(std::max((pop_count(board.b_pawns & mask) - 1), 0),
                                    Evaluation::characteristic_t::DOUBLED_PAWNS);
        mask <<= 1;
    }
}

void Evaluation::passed_pawns() {
    uint64_t frontspans = compute_pawn_frontspans_w(board.w_pawns, board.b_pawns);

    uint64_t prom_square_mask = BB_FILE_A & BB_RANK_8;
    uint64_t file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        if (prom_square_mask & frontspans) {
            int n = 1;
            uint64_t left_file = (frontspans & file_mask) >> 1;
            uint64_t right_file = (frontspans & file_mask) << 1;

            n += ((left_file & (board.b_pawns | BB_FILE_H)) == 0);
            n += ((right_file & (board.b_pawns | BB_FILE_A)) == 0);
            accumulate_characteristic_w(n, Evaluation::characteristic_t::PASSED_PAWN);
        }
        prom_square_mask <<= 1;
        file_mask <<= 1;
    }

    frontspans = compute_pawn_frontspans_b(board.b_pawns, board.w_pawns);
    prom_square_mask = BB_FILE_A & BB_RANK_1;
    file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        if (prom_square_mask & frontspans) {
            int n = 1;
            uint64_t left_file = (frontspans & file_mask) >> 1;
            uint64_t right_file = (frontspans & file_mask) << 1;

            n += ((left_file & (board.w_pawns | BB_FILE_H)) == 0);
            n += ((right_file & (board.w_pawns | BB_FILE_A)) == 0);
            accumulate_characteristic_b(n, Evaluation::characteristic_t::PASSED_PAWN);
        }
        prom_square_mask <<= 1;
        file_mask <<= 1;
    }
}

void Evaluation::backward_pawns() {
    uint64_t backspans = compute_pawn_backspans_w(board.w_pawns, 0ULL);
    uint64_t file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = backspans & file_mask;
        if (rear_file) {
            /** If there exists a pawn in the file */
            uint64_t left_potential_guards = (rear_file >> 1) & ~BB_FILE_H;
            uint64_t right_potential_guards = (rear_file << 1) & BB_FILE_A;
            bool left_guardless = ((left_potential_guards & board.w_pawns) == 0);
            bool right_guardless = ((right_potential_guards & board.w_pawns) == 0);
            accumulate_characteristic_w((int) (left_guardless & right_guardless),
                                        Evaluation::characteristic_t::BACKWARD_PAWN);
        }
        file_mask <<= 1;
    }
    backspans = compute_pawn_backspans_b(board.b_pawns, 0ULL);
    file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = backspans & file_mask;
        if (rear_file) {
            uint64_t left_potential_guards = (rear_file >> 1) & ~BB_FILE_H;
            uint64_t right_potential_guards = (rear_file << 1) & BB_FILE_A;
            bool left_guardless = ((left_potential_guards & board.b_pawns) == 0);
            bool right_guardless = ((right_potential_guards & board.b_pawns) == 0);
            accumulate_characteristic_b((int) (left_guardless & right_guardless),
                                        Evaluation::characteristic_t::BACKWARD_PAWN);
        }
        file_mask <<= 1;
    }
}

void Evaluation::knight_activity() {
    uint64_t knights = get_knight_mask_setwise(board.w_knights);

    int n = pop_count(knights & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    int32_t knights_ctrl = 0;
    while (knights) {
        int i = pull_lsb(&knights);
        knights_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += knights_ctrl / 3;

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
}

void Evaluation::rook_activity() {
    uint64_t data = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    /** Detection of connected rooks */
    accumulate_characteristic_w(std::max((pop_count(data & board.w_rooks) - 1), 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);

    int n = pop_count(data & b_king_vulnerabilities);
    midgame_score += n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score += n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    /** Evaluates board-control by rooks using the guard-heuristic.*/
    int32_t rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score += rook_ctrl / 5;
    /** The following repeats the same score for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    accumulate_characteristic_b(std::max(pop_count(data & board.b_rooks) - 1, 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);

    n = pop_count(data & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    rook_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        rook_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= rook_ctrl / 5;
    evaluate_psqt_b(_get_reverse_bb(board.b_rooks), Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
}

void Evaluation::queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = get_queen_rays_setwise(board.w_queens,
                                           (~board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    accumulate_characteristic_w(std::max(pop_count(data & board.w_rooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);

    /** Detects Queen-Bishop batteries. */
    accumulate_characteristic_w(std::max(pop_count(data & board.w_bishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);

    int n = pop_count(data & b_king_vulnerabilities);
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

    /** Following code duplicates the above functionality for black */
    data = get_queen_rays_setwise(board.b_queens, (~board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    accumulate_characteristic_b(std::max(pop_count(data & board.b_rooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);
    accumulate_characteristic_b(std::max(pop_count(data & board.b_bishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);

    n = pop_count(data & w_king_vulnerabilities);
    midgame_score -= n * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    endgame_score -= n * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];

    queen_ctrl = 0;
    while (data) {
        int i = pull_lsb(&data);
        queen_ctrl += Weights::board_ctrl_tb[i];
    }
    midgame_score -= queen_ctrl / 9;
}

void Evaluation::king_placement() {
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

void Evaluation::accumulate_psqts() {
    evaluate_psqt_w(board.w_pawns, Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    evaluate_psqt_w(board.w_knights, Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    evaluate_psqt_w(board.w_bishops, Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    evaluate_psqt_w(board.w_rooks, Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    evaluate_psqt_w(board.w_queens, Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    evaluate_psqt_w(board.w_king, Weights::KING_PSQT, Weights::Endgame::KING_PSQT);

    evaluate_psqt_b(_get_reverse_bb(board.b_pawns), Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    evaluate_psqt_b(_get_reverse_bb(board.b_knights), Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    evaluate_psqt_b(_get_reverse_bb(board.b_bishops), Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    evaluate_psqt_b(_get_reverse_bb(board.b_rooks), Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    evaluate_psqt_b(_get_reverse_bb(board.b_queens), Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    evaluate_psqt_b(_get_reverse_bb(board.b_king), Weights::KING_PSQT, Weights::Endgame::KING_PSQT);
}

void Evaluation::evaluate_psqt_w(uint64_t bb, const int32_t mg_psqt[64], const int32_t eg_psqt[64]) {
    while (bb) {
        int i = pull_lsb(&bb);
        midgame_score += mg_psqt[i];
        endgame_score += eg_psqt[i];
    }
}

void Evaluation::evaluate_psqt_b(uint64_t bb, const int32_t mg_psqt[64], const int32_t eg_psqt[64]) {
    while (bb) {
        int i = pull_lsb(&bb);
        midgame_score -= mg_psqt[i];
        endgame_score -= eg_psqt[i];
    }
}

void Evaluation::accumulate_characteristic_w(int n, Evaluation::characteristic_t type) {
    midgame_score += (n * Weights::POSITIONAL[static_cast<size_t> (type)]);
    endgame_score += (n * Weights::Endgame::POSITIONAL[static_cast<size_t> (type)]);
}

void Evaluation::accumulate_characteristic_b(int n, Evaluation::characteristic_t type) {
    midgame_score -= (n * Weights::POSITIONAL[static_cast<size_t> (type)]);
    endgame_score -= (n * Weights::Endgame::POSITIONAL[static_cast<size_t> (type)]);
}