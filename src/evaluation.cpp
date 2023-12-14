#include "bitboard.h"
#include "evaluation.h"
#include "movegen.h"
#include "weights.h"

#include <algorithm>
#include <cmath>
#include <cstring>

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

Evaluation::Evaluation(const Bitboard *board) {
    this->board = board;
}

void Evaluation::reset() {
    midgame_score = 0;
    endgame_score = 0;
    w_pawn_rearspans = whitePawnsRearspan(this->board->wPawns, 0ULL);
    b_pawn_rearspans = blackPawnsRearspan(this->board->bPawns, 0ULL);
    countOpenFiles();
    whiteSpaceBonus();
    blackSpaceBonus();
    std::memset(square_guard_values, 0, 64 * sizeof(int8_t));
    progression = gamePhase();
}

int32_t Evaluation::weightedScore() const {
    return (1 - 2 * (this->board->getTurn() == BLACK)) *
           (int32_t) std::round(midgame_score * progression + endgame_score * (1 - progression));
}

/**
 * Computes a bitboard of vulnerable squares around the king. Opponent gets bonus of pieces hit these squares.
 * @param king bitboard with king
 * @param barriers bitboard with barriers that protect the king (could be pieces, pawns, or nothing)
 * @return a bitboard of vulnerable squares around the king.
 */

uint64_t Evaluation::kingVulnerabilities(uint64_t king, uint64_t barriers) {
    /** Highlights the squares around the king */
    uint64_t king_occ = 0, temp = (king << 1) & ~Bitboard::BB_FILE_A;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~barriers;
    temp |= ((temp << 1) & ~Bitboard::BB_FILE_A);
    temp &= ~barriers;
    king_occ |= temp;
    temp = (king >> 1) & ~Bitboard::BB_FILE_H;
    temp |= (temp >> 8) | (temp << 8);
    temp &= ~barriers;
    temp |= ((temp >> 1) & ~Bitboard::BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;

    temp = king >> 8;
    temp |= ((temp << 1) & ~Bitboard::BB_FILE_A) | ((temp >> 1) & ~Bitboard::BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;
    temp >>= 8;
    temp &= ~barriers;
    king_occ |= temp;

    temp = king << 8;
    temp |= ((temp << 1) & ~Bitboard::BB_FILE_A) | ((temp >> 1) & ~Bitboard::BB_FILE_H);
    temp &= ~barriers;
    king_occ |= temp;
    temp <<= 8;
    temp &= ~barriers;
    king_occ |= temp;
    return king_occ;
}

uint64_t Evaluation::whitePawnsFrontspan(uint64_t pawns_bb, uint64_t occupied_bb) {
    uint64_t frontspans = 0ULL;
    do {
        pawns_bb <<= 8;
        pawns_bb ^= (pawns_bb & occupied_bb);
        frontspans |= pawns_bb;
    } while (pawns_bb);
    return frontspans;
}

uint64_t Evaluation::blackPawnsFrontspan(uint64_t pawns_bb, uint64_t occupied_bb) {
    uint64_t frontspans = 0ULL;
    do {
        pawns_bb >>= 8;
        pawns_bb ^= (pawns_bb & occupied_bb);
        frontspans |= pawns_bb;
    } while (pawns_bb);
    return frontspans;
}

uint64_t Evaluation::whitePawnsRearspan(uint64_t pawns_bb, uint64_t occupied_bb) {
    return blackPawnsFrontspan(pawns_bb, occupied_bb);
}

uint64_t Evaluation::blackPawnsRearspan(uint64_t pawns_bb, uint64_t occupied_bb) {
    return whitePawnsFrontspan(pawns_bb, occupied_bb);
}

double Evaluation::gamePhase() {
    phase = 0;
    phase += BitUtils::popCount(this->board->wPawns | this->board->bPawns) * Weights::PAWN_PHASE;
    phase += BitUtils::popCount(this->board->wKnights | this->board->bKnights) * Weights::KNIGHT_PHASE;
    phase += BitUtils::popCount(this->board->wBishops | this->board->bBishops) * Weights::BISHOP_PHASE;
    phase += BitUtils::popCount(this->board->wRooks | this->board->bRooks) * Weights::ROOK_PHASE;
    phase += BitUtils::popCount(this->board->wQueens | this->board->bQueens) * Weights::QUEEN_PHASE;
    return ((double) phase) / Weights::TOTAL_PHASE;
}

void Evaluation::whiteSpaceBonus() {
    int n_pieces = BitUtils::popCount(this->board->wRooks) + BitUtils::popCount(this->board->wKnights)
                   + BitUtils::popCount(this->board->wBishops) + BitUtils::popCount(this->board->wQueens);
    w_space_bonus = n_pieces - n_open_files;
}

void Evaluation::blackSpaceBonus() {
    int n_pieces = BitUtils::popCount(this->board->bRooks) + BitUtils::popCount(this->board->bKnights) +
                   BitUtils::popCount(this->board->bBishops) + BitUtils::popCount(this->board->bQueens);
    b_space_bonus = n_pieces - n_open_files;
}

void Evaluation::countOpenFiles() {
    uint64_t file_mask = Bitboard::BB_FILE_A;
    int32_t n = 0;
    for (int i = 0; i < 8; ++i) {
        n += ((file_mask & this->board->occupied) == 0);
        file_mask <<= 1;
    }
    n_open_files = n;
}

int32_t Evaluation::evaluate() {
    reset();
    materialScore();
    PSQTs();

    pawnStructure();
    passedPawns();
    doubledPawns();
    backwardPawns();
    rook_activity();
    queen_activity();
    evaluateSpace();
    // TODO: New (distance to pawns, opposition, etc), Outpost Squares,
    if (phase < 6) {
        king_placement();
    }
    int32_t s = weightedScore();
    //std::cout << "score: " << s << '\n';
    return s;
}

void Evaluation::materialScore() {
    /** White Material Score */
    int n = BitUtils::popCount(this->board->wPawns);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    n = BitUtils::popCount(this->board->wKnights);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    n = BitUtils::popCount(this->board->wBishops);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    n = BitUtils::popCount(this->board->wRooks);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    n = BitUtils::popCount(this->board->wQueens);
    midgame_score += n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
    endgame_score += n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];

    /** Black material score */
    n = BitUtils::popCount(this->board->bPawns);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_PAWN)];
    n = BitUtils::popCount(this->board->bKnights);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_KNIGHT)];
    n = BitUtils::popCount(this->board->bBishops);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_BISHOP)];
    n = BitUtils::popCount(this->board->bRooks);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_ROOK)];
    n = BitUtils::popCount(this->board->bQueens);
    midgame_score -= n * Weights::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
    endgame_score -= n * Weights::Endgame::MATERIAL[static_cast<size_t> (piece_t::BLACK_QUEEN)];
}

void Evaluation::pawnStructure() {
    uint64_t pawn_attacks = MoveGen::get_pawn_attacks_setwise(this->board->wPawns, WHITE);
    whiteCharacteristic(BitUtils::popCount(pawn_attacks & this->board->wPawns),
                                Evaluation::characteristic_t::PAWN_CHAIN);

    pawn_attacks = MoveGen::get_pawn_attacks_setwise(this->board->bPawns, BLACK);
    blackCharacteristic(BitUtils::popCount(pawn_attacks & this->board->bPawns), Evaluation::characteristic_t::PAWN_CHAIN);
}

void Evaluation::doubledPawns() {
    uint64_t mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        /** Calculates (number of pawns in a file - 1) and penalizes accordingly */
        whiteCharacteristic(std::max(BitUtils::popCount(this->board->wPawns & mask) - 1, 0),
                                    Evaluation::characteristic_t::DOUBLED_PAWNS);
        /** Shifts bitmask one file right */
        mask <<= 1;
    }
    mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        blackCharacteristic(std::max((BitUtils::popCount(this->board->bPawns & mask) - 1), 0),
                                    Evaluation::characteristic_t::DOUBLED_PAWNS);
        mask <<= 1;
    }
}

void Evaluation::passedPawns() {
    uint64_t frontspans = whitePawnsFrontspan(this->board->wPawns, this->board->bPawns);

    uint64_t promSquare_mask = Bitboard::BB_FILE_A & Bitboard::BB_RANK_8;
    uint64_t file_mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        if (promSquare_mask & frontspans) {
            int n = 1;
            uint64_t left_file = (frontspans & file_mask) >> 1;
            uint64_t right_file = (frontspans & file_mask) << 1;

            n += ((left_file & (this->board->bPawns | Bitboard::BB_FILE_H)) == 0);
            n += ((right_file & (this->board->bPawns | Bitboard::BB_FILE_A)) == 0);
            whiteCharacteristic(n, Evaluation::characteristic_t::PASSED_PAWN);
        }
        promSquare_mask <<= 1;
        file_mask <<= 1;
    }

    frontspans = blackPawnsFrontspan(this->board->bPawns, this->board->wPawns);
    promSquare_mask = Bitboard::BB_FILE_A & Bitboard::BB_RANK_1;
    file_mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        if (promSquare_mask & frontspans) {
            int n = 1;
            uint64_t left_file = (frontspans & file_mask) >> 1;
            uint64_t right_file = (frontspans & file_mask) << 1;

            n += ((left_file & (this->board->wPawns | Bitboard::BB_FILE_H)) == 0);
            n += ((right_file & (this->board->wPawns | Bitboard::BB_FILE_A)) == 0);
            blackCharacteristic(n, Evaluation::characteristic_t::PASSED_PAWN);
        }
        promSquare_mask <<= 1;
        file_mask <<= 1;
    }
}

void Evaluation::backwardPawns() {
    uint64_t rearspans = whitePawnsRearspan(this->board->wPawns, 0ULL);
    uint64_t file_mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = rearspans & file_mask;
        if (rear_file) {
            /** If there exists a pawn in the file */
            uint64_t left_potential_guards = (rear_file >> 1) & ~Bitboard::BB_FILE_H;
            uint64_t right_potential_guards = (rear_file << 1) & Bitboard::BB_FILE_A;
            bool left_guardless = ((left_potential_guards & this->board->wPawns) == 0);
            bool right_guardless = ((right_potential_guards & this->board->wPawns) == 0);
            whiteCharacteristic((int) (left_guardless & right_guardless),
                                        Evaluation::characteristic_t::BACKWARD_PAWN);
        }
        file_mask <<= 1;
    }
    rearspans = blackPawnsRearspan(this->board->bPawns, 0ULL);
    file_mask = Bitboard::BB_FILE_A;
    for (int i = 0; i < 8; ++i) {
        uint64_t rear_file = rearspans & file_mask;
        if (rear_file) {
            uint64_t left_potential_guards = (rear_file >> 1) & ~Bitboard::BB_FILE_H;
            uint64_t right_potential_guards = (rear_file << 1) & Bitboard::BB_FILE_A;
            bool left_guardless = ((left_potential_guards & this->board->bPawns) == 0);
            bool right_guardless = ((right_potential_guards & this->board->bPawns) == 0);
            blackCharacteristic((int) (left_guardless & right_guardless),
                                        Evaluation::characteristic_t::BACKWARD_PAWN);
        }
        file_mask <<= 1;
    }
}

void Evaluation::rook_activity() {
    uint64_t data = MoveGen::get_rook_rays_setwise(this->board->wRooks, ~(this->board->occupied ^ this->board->wRooks));
    /** Detection of connected rooks */
    whiteCharacteristic(std::max((BitUtils::popCount(data & this->board->wRooks) - 1), 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);

    /** The following repeats the same score for black*/
    data = MoveGen::get_rook_rays_setwise(this->board->bRooks, ~(this->board->occupied ^ this->board->bRooks));
    blackCharacteristic(std::max(BitUtils::popCount(data & this->board->bRooks) - 1, 0),
                                Evaluation::characteristic_t::CONNECTED_ROOKS);
}

void Evaluation::queen_activity() {
    /**
     *  @var uint64_t data Stores a bitboard of all squares hit by any white queen.
     */
    uint64_t data = MoveGen::get_queen_rays_setwise(this->board->wQueens,
                                           ~(this->board->occupied ^ this->board->wQueens ^ this->board->wRooks ^ this->board->wBishops));

    /** Detects Queen-Rook batteries. */
    whiteCharacteristic(std::max(BitUtils::popCount(data & this->board->wRooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);

    /** Detects Queen-Bishop batteries. */
    whiteCharacteristic(std::max(BitUtils::popCount(data & this->board->wBishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);

    /** Evaluates queen placement using piece-square table */

    /** Following code duplicates the above functionality for black */
    data = MoveGen::get_queen_rays_setwise(this->board->bQueens, ~(this->board->occupied ^ this->board->bQueens ^ this->board->bRooks ^ this->board->bBishops));
    blackCharacteristic(std::max(BitUtils::popCount(data & this->board->bRooks) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_ROOK);
    blackCharacteristic(std::max(BitUtils::popCount(data & this->board->bBishops) - 1, 0),
                                Evaluation::characteristic_t::QUEEN_BISHOP);
}

void Evaluation::king_placement() {
    /**
     *  Encourages minimizing king distance toward the endgame, by giving a "bonus" inversely proportional to mutual
     *  king distance to the side whose king is closer to the center. Award's an additional "bonus" inversely
     *  proportional to king distance from the edge of the board under the same conditions.
     *
     *  The goal is to incentivize the computer to force an opposing king toward the corner during the endgame.
     */

    int w_file = Bitboard::fileOf(this->board->wKingSquare), w_rank = Bitboard::rankOf(this->board->wKingSquare / 8);
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

    int b_file = Bitboard::fileOf(this->board->bKingSquare), b_rank = Bitboard::rankOf(this->board->bKingSquare);
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

void Evaluation::evaluateSpace() {
    /** Accumulate guard values of white pieces */
    uint64_t attacks = MoveGen::BB_KING_ATTACKS[this->board->wKingSquare];
    uint64_t king_vulnerabilities = this->kingVulnerabilities(this->board->bKing, this->board->bPawns);
    int32_t mg_king_danger = 0;
    int32_t eg_king_danger = 0;
    int n_attackers = 0;

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_KING];
    }
    attacks = MoveGen::get_queen_rays_setwise(this->board->wQueens, ~this->board->occupied) & (~this->board->wQueens);
    this->accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_QUEEN];
    }

    attacks = MoveGen::get_rook_rays_setwise(this->board->wRooks, ~this->board->occupied) & (~this->board->wRooks);
    this->accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_ROOK];
    }
    attacks = MoveGen::get_bishop_rays_setwise(this->board->wBishops, ~this->board->occupied) & (~this->board->wBishops);
    this->accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_BISHOP];
    }
    attacks = MoveGen::get_knight_mask_setwise(this->board->wKnights);
    this->accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_KNIGHT];
    }
    attacks = MoveGen::get_pawn_attacks_setwise(board->wPawns, WHITE);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    midgame_score += n_attackers * mg_king_danger;
    endgame_score += n_attackers * eg_king_danger;

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] += Weights::GUARD_VALUE[piece_t::BLACK_PAWN];
    }

    mg_king_danger = 0;
    eg_king_danger = 0;
    n_attackers = 0;
    king_vulnerabilities = kingVulnerabilities(this->board->wKing, this->board->wPawns);

    /** Accumulate guard values of black pieces */
    attacks = MoveGen::BB_KING_ATTACKS[this->board->bKingSquare];
    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_KING];
    }
    attacks = MoveGen::get_queen_rays_setwise(this->board->bQueens, ~this->board->occupied) & (~this->board->bQueens);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_QUEEN];
    }
    attacks = MoveGen::get_rook_rays_setwise(this->board->bRooks, ~this->board->occupied) & (~this->board->bRooks);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_ROOK];
    }
    attacks = MoveGen::get_bishop_rays_setwise(this->board->bBishops, ~this->board->occupied) & (~this->board->bBishops);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_BISHOP];
    }
    attacks = MoveGen::get_knight_mask_setwise(this->board->bKnights);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
        square_guard_values[i] -= Weights::GUARD_VALUE[piece_t::BLACK_KNIGHT];
    }
    attacks = MoveGen::get_pawn_attacks_setwise(this->board->bPawns, BLACK);
    accumulateKingThreats(n_attackers, mg_king_danger, eg_king_danger, attacks, king_vulnerabilities);

    while (attacks) {
        int i = BitUtils::pullLSB(&attacks);
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

    uint64_t w_king_surroundings = kingVulnerabilities(this->board->wKing, 0ULL);
    uint64_t b_king_surroundings = kingVulnerabilities(this->board->bKing, 0ULL);
    uint64_t examinedSquare = 1ULL;
    for (int i = 0; i < 64; ++i) {
        /** Sets square_guard_values[i] = sign(square_guard_values[i]) */
        square_guard_values[i] = (square_guard_values[i] > 0) - (square_guard_values[i] < 0);
        /** If a square is controlled by black, bonus will be b_space_bonus, and vice versa. */
        int bonus = (square_guard_values[i] > 0) * w_space_bonus + (square_guard_values[i] < 0) * b_space_bonus;
        /** If a square is controlled by black, opponent_rear will be white pawn's rearspan, and vice versa */
        uint64_t opponent_rear = ((square_guard_values[i] > 0) * b_pawn_rearspans) |
                                 ((square_guard_values[i] < 0) * w_pawn_rearspans);
        /** If the current square being examined is behind opponent's pawns, double the bonus. */
        bonus <<= ((examinedSquare & opponent_rear) != 0);
        midgame_score += square_guard_values[i] * (Weights::board_ctrl_tb[i] * bonus);
        midgame_score += (square_guard_values[i] > 0) * ((examinedSquare & b_king_surroundings) != 0) * KING_THREAT;
        midgame_score -= (square_guard_values[i] < 0) * ((examinedSquare & w_king_surroundings) != 0) * KING_THREAT;
        examinedSquare <<= 1;
    }
}

void Evaluation::accumulateKingThreats(int &n_attackers, int32_t &mg_score, int32_t &eg_score, uint64_t attacks,
                                         uint64_t king) {
    int pop_cnt = BitUtils::popCount(attacks & king);
    mg_score += pop_cnt * Weights::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    eg_score += pop_cnt * Weights::Endgame::POSITIONAL[Evaluation::characteristic_t::KING_THREAT];
    n_attackers += (pop_cnt > 0);
}

void Evaluation::PSQTs() {
    whitePSQT(this->board->wPawns, Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    whitePSQT(this->board->wKnights, Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    whitePSQT(this->board->wBishops, Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    whitePSQT(this->board->wRooks, Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    whitePSQT(this->board->wQueens, Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    whitePSQT(this->board->wKing, Weights::KING_PSQT, Weights::Endgame::KING_PSQT);

    blackPSQT(BitUtils::flipBitboardVertical(this->board->bPawns), Weights::PAWN_PSQT, Weights::Endgame::PAWN_PSQT);
    blackPSQT(BitUtils::flipBitboardVertical(this->board->bKnights), Weights::KNIGHT_PSQT, Weights::Endgame::KNIGHT_PSQT);
    blackPSQT(BitUtils::flipBitboardVertical(this->board->bBishops), Weights::BISHOP_PSQT, Weights::Endgame::BISHOP_PSQT);
    blackPSQT(BitUtils::flipBitboardVertical(this->board->bRooks), Weights::ROOK_PSQT, Weights::Endgame::ROOK_PSQT);
    blackPSQT(BitUtils::flipBitboardVertical(this->board->bQueens), Weights::QUEEN_PSQT, Weights::Endgame::QUEEN_PSQT);
    blackPSQT(BitUtils::flipBitboardVertical(this->board->bKing), Weights::KING_PSQT, Weights::Endgame::KING_PSQT);
}

void Evaluation::whitePSQT(uint64_t bb, const int32_t mg_psqt[64], const int32_t eg_psqt[64]) {
    while (bb) {
        int i = BitUtils::pullLSB(&bb);
        midgame_score += mg_psqt[i];
        endgame_score += eg_psqt[i];
    }
}

void Evaluation::blackPSQT(uint64_t bb, const int32_t mg_psqt[64], const int32_t eg_psqt[64]) {
    while (bb) {
        int i = BitUtils::pullLSB(&bb);
        midgame_score -= mg_psqt[i];
        endgame_score -= eg_psqt[i];
    }
}

void Evaluation::whiteCharacteristic(int n, Evaluation::characteristic_t type) {
    midgame_score += (n * Weights::POSITIONAL[static_cast<size_t> (type)]);
    endgame_score += (n * Weights::Endgame::POSITIONAL[static_cast<size_t> (type)]);
}

void Evaluation::blackCharacteristic(int n, Evaluation::characteristic_t type) {
    midgame_score -= (n * Weights::POSITIONAL[static_cast<size_t> (type)]);
    endgame_score -= (n * Weights::Endgame::POSITIONAL[static_cast<size_t> (type)]);
}