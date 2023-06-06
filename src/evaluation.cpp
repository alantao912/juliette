#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <algorithm>
#include <cmath>
#include <cstring>

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
    w_pawn_rearspans = compute_pawn_rearspans_w(board.w_pawns, 0ULL);
    b_pawn_rearspans = compute_pawn_rearspans_b(board.b_pawns, 0ULL);
    compute_n_open_files();
    compute_space_bonus_w();
    compute_space_bonus_b();
    std::memset(square_guard_values, 0, 64 * sizeof(int8_t));
    progression = compute_progression();
}

int32_t Evaluation::compute_score() const {
    return (1 - 2 * (board.turn == BLACK)) *
           (int32_t) std::round(midgame_score * progression + endgame_score * (1 - progression));
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

uint64_t Evaluation::compute_pawn_rearspans_w(uint64_t pawns_bb, uint64_t occupied_bb) {
    return compute_pawn_frontspans_b(pawns_bb, occupied_bb);
}

uint64_t Evaluation::compute_pawn_rearspans_b(uint64_t pawns_bb, uint64_t occupied_bb) {
    return compute_pawn_frontspans_w(pawns_bb, occupied_bb);
}

double Evaluation::compute_progression() {
    phase = 0;
    phase += pop_count(board.w_pawns | board.b_pawns) * Weights::PAWN_PHASE;
    phase += pop_count(board.w_knights | board.b_knights) * Weights::KNIGHT_PHASE;
    phase += pop_count(board.w_bishops | board.b_bishops) * Weights::BISHOP_PHASE;
    phase += pop_count(board.w_rooks | board.b_rooks) * Weights::ROOK_PHASE;
    phase += pop_count(board.w_queens | board.b_queens) * Weights::QUEEN_PHASE;
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
    rook_activity();
    queen_activity();
    evaluate_space();
    // TODO: New (distance to pawns, opposition, etc), Outpost Squares,
    if (phase < 6) {
        king_placement();
    }
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

    pawn_attacks = get_pawn_attacks_setwise(BLACK);
    accumulate_characteristic_b(pop_count(pawn_attacks & board.b_pawns), Evaluation::characteristic_t::PAWN_CHAIN);
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
    uint64_t rearspans = compute_pawn_rearspans_w(board.w_pawns, 0ULL);
    uint64_t file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = rearspans & file_mask;
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
    rearspans = compute_pawn_rearspans_b(board.b_pawns, 0ULL);
    file_mask = BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = rearspans & file_mask;
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

void Evaluation::rook_activity() {
    uint64_t data = get_rook_rays_setwise(board.w_rooks, ~(board.occupied ^ board.w_rooks));
    /** Detection of connected rooks */
    accumulate_characteristic_w(std::max((pop_count(data & board.w_rooks) - 1), 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);

    /** The following repeats the same score for black*/
    data = get_rook_rays_setwise(board.b_rooks, ~(board.occupied ^ board.b_rooks));
    accumulate_characteristic_b(std::max(pop_count(data & board.b_rooks) - 1, 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);
}

void Evaluation::queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = get_queen_rays_setwise(board.w_queens,
                                           ~(board.occupied ^ board.w_queens ^ board.w_rooks ^ board.w_bishops));

    /** Detects Queen-Rook batteries. */
    accumulate_characteristic_w(std::max(pop_count(data & board.w_rooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);

    /** Detects Queen-Bishop batteries. */
    accumulate_characteristic_w(std::max(pop_count(data & board.w_bishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);

    /** Evaluates queen placement using piece-square table */

    /** Following code duplicates the above functionality for black */
    data = get_queen_rays_setwise(board.b_queens, ~(board.occupied ^ board.b_queens ^ board.b_rooks ^ board.b_bishops));
    accumulate_characteristic_b(std::max(pop_count(data & board.b_rooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);
    accumulate_characteristic_b(std::max(pop_count(data & board.b_bishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);
}

void Evaluation::king_placement() {
    /**
     *  Encourages minimizing king distance toward the endgame, by giving a "bonus" inversely proportional to mutual
     *  king distance to the side whose king is closer to the center. Award's an additional "bonus" inversely
     *  proportional to king distance from the edge of the board under the same conditions.
     *
     *  The goal is to incentivize juliette to force an opposing king toward the corner during the endgame.
     */

    int w_file = file_of(board.w_king_square), w_rank = rank_of(board.w_king_square / 8);
    int wEdgeDist = std::min(std::min(w_file, 7 - w_file), std::min(w_rank, 7 - w_rank));

    int wCenterDist;
    {
        /** Calculates white king's distance to center */
        int dx = std::min(std::abs(3 - w_rank), w_rank - 4);
        dx *= dx;
        int dy = std::min(std::abs(3 - w_file), w_file - 4);
        dy *= dy;
        wCenterDist = dx + dy;
    }

    int b_file = file_of(board.b_king_square), b_rank = rank_of(board.b_king_square);
    int bEdgeDist = std::min(std::min(b_file, 7 - b_file), std::min(b_rank, 7 - b_rank));

    int bCenterDist;
    {
        /** Calculates black king's distance to center */
        int dx = std::min(std::abs(3 - b_rank), b_rank - 4);
        dx *= dx;
        int dy = std::min(std::abs(3 - b_file), b_file - 4);
        dy *= dy;
        bCenterDist = dx + dy;
    }

    int kingVDiff = std::abs(b_rank - w_rank), kingHDiff = std::abs(b_file - w_file);
    /** King distance bonus. Award bonus inversely proportional to king distance, to side whose king is closer to the center */
    int kingDistBonus = (8 - std::max(kingVDiff, kingHDiff)) * Weights::Endgame::KING_DIST;
    /** Difference between black king distance to center, and white king distance to center. */
    int distDiff = bCenterDist - wCenterDist;

    /** White is closer to the center */
    int king_edge_bonus = (3 - bEdgeDist) * Weights::Endgame::KING_EDGE;
    endgame_score += (distDiff > 0) * (kingDistBonus + king_edge_bonus);
    /* Black is closer to the center */
    king_edge_bonus = (3 - wEdgeDist) * Weights::Endgame::KING_EDGE;
    endgame_score -= (distDiff < 0) * (kingDistBonus + king_edge_bonus);
}

void Evaluation::king_opposition() {
    // TODO: Implement opposition detection. Direct opposition, Distant opposition, Diagonal Opposition
}

void Evaluation::evaluate_space() {
    /** Accumulate guard values of white pieces */
    uint64_t attacks = BB_KING_ATTACKS[board.w_king_square];
    uint64_t king_vulnerabilities = compute_king_vulnerabilities(board.b_king, board.b_pawns);
    int32_t mg_king_danger = 0;
    int32_t eg_king_danger = 0;
    int n_attackers = 0;

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_KING];
    }
    attacks = get_queen_rays_setwise(board.w_queens, ~board.occupied) & (~board.w_queens);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_QUEEN];
    }

    attacks = get_rook_rays_setwise(board.w_rooks, ~board.occupied) & (~board.w_rooks);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_ROOK];
    }
    attacks = get_bishop_rays_setwise(board.w_bishops, ~board.occupied) & (~board.w_bishops);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_BISHOP];
    }
    attacks = get_knight_mask_setwise(board.w_knights);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_KNIGHT];
    }
    attacks = get_pawn_attacks_setwise(WHITE);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    midgame_score += n_attackers * mg_king_danger;
    endgame_score += n_attackers * eg_king_danger;

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_PAWN];
    }

    mg_king_danger = 0;
    eg_king_danger = 0;
    n_attackers = 0;
    king_vulnerabilities = compute_king_vulnerabilities(board.w_king, board.w_pawns);

    /** Accumulate guard values of black pieces */
    attacks = BB_KING_ATTACKS[board.b_king_square];
    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_KING];
    }
    attacks = get_queen_rays_setwise(board.b_queens, ~board.occupied) & (~board.b_queens);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_QUEEN];
    }
    attacks = get_rook_rays_setwise(board.b_rooks, ~board.occupied) & (~board.b_rooks);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_ROOK];
    }
    attacks = get_bishop_rays_setwise(board.b_bishops, ~board.occupied) & (~board.b_bishops);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_BISHOP];
    }
    attacks = get_knight_mask_setwise(board.b_knights);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_KNIGHT];
    }
    attacks = get_pawn_attacks_setwise(BLACK);
    accumulate_king_threats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = pull_lsb(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_PAWN];
    }

    midgame_score -= n_attackers * mg_king_danger;
    endgame_score -= n_attackers * eg_king_danger;

    /** Determine who has stronger control over each square. */
    /**
     * For each square, if white's guard value exceeds black's guard value. Then that particular square
     * "is in white's control", and vice versa.
     *
     * Implemented as, guard_values[i] = sgn(guard_values[i]).
     */

    uint64_t w_king_surroundings = compute_king_vulnerabilities(board.w_king, 0ULL);
    uint64_t b_king_surroundings = compute_king_vulnerabilities(board.b_king, 0ULL);
    uint64_t examined_square = 1ULL;
    for (int i = 0; i < 64; ++i) {
        /** Sets square_guard_values[i] = sign(square_guard_values[i]) */
        square_guard_values[i] = (square_guard_values[i] > 0) - (square_guard_values[i] < 0);
        /** If a square is controlled by black, bonus will be b_space_bonus, and vice versa. */
        int bonus = (square_guard_values[i] > 0) * w_space_bonus + (square_guard_values[i] < 0) * b_space_bonus;
        /** If a square is controlled by black, opponent_rear will be white pawn's rearspan, and vice versa */
        uint64_t opponent_rear = ((square_guard_values[i] > 0) * b_pawn_rearspans) |
                                 ((square_guard_values[i] < 0) * w_pawn_rearspans);
        /** If the current square being examined is behind opponent's pawns, double the bonus. */
        bonus <<= ((examined_square & opponent_rear) != 0);
        midgame_score += square_guard_values[i] * (Weights::board_ctrl_tb[i] * bonus);
        midgame_score += (square_guard_values[i] > 0) * ((examined_square & b_king_surroundings) != 0) * KING_THREAT;
        midgame_score -= (square_guard_values[i] < 0) * ((examined_square & w_king_surroundings) != 0) * KING_THREAT;
        examined_square <<= 1;
    }
}

void Evaluation::accumulate_king_threats(int &n_attackers, int32_t &mg_score, int32_t &eg_score, uint64_t attacks,
                                         uint64_t king) {
    int pop_cnt = pop_count(attacks & king);
    mg_score += pop_cnt * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    eg_score += pop_cnt * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    n_attackers += (pop_cnt > 0);
}

void Evaluation::accumulate_psqts() {
    evaluate_psqt_w(board.w_pawns, Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    evaluate_psqt_w(board.w_knights, Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    evaluate_psqt_w(board.w_bishops, Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    evaluate_psqt_w(board.w_rooks, Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    evaluate_psqt_w(board.w_queens, Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    evaluate_psqt_w(board.w_king, Weights::KING_PSQT, Weights::Endgame::KING_PSQT);

    evaluate_psqt_b(vflip_bb(board.b_pawns), Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    evaluate_psqt_b(vflip_bb(board.b_knights), Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    evaluate_psqt_b(vflip_bb(board.b_bishops), Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    evaluate_psqt_b(vflip_bb(board.b_rooks), Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    evaluate_psqt_b(vflip_bb(board.b_queens), Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    evaluate_psqt_b(vflip_bb(board.b_king), Weights::KING_PSQT, Weights::Endgame::KING_PSQT);
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