#pragma once

#include <cstdint>
#include "util.h"

struct Evaluation {
    /** Determine board characteristics */

    static uint64_t kingVulnerabilities(uint64_t king_bb, uint64_t barriers);

public:

    int32_t evaluate();

    void reset();

    Evaluation(const Bitboard *);

private:

    enum characteristic_t {
        PAWN_CHAIN, DOUBLED_PAWNS, CONNECTED_ROOKS, QUEEN_ROOK, QUEEN_BISHOP, KING_THREAT, PASSED_PAWN, BACKWARD_PAWN
    };

    const Bitboard *board;

    uint16_t phase;

    uint64_t w_pawn_rearspans, b_pawn_rearspans;

    int32_t n_open_files;
    int32_t w_space_bonus, b_space_bonus;
    int32_t midgame_score, endgame_score;

    int8_t square_guard_values[64];

    double progression;

    int32_t weightedScore() const;

    static uint64_t whitePawnsFrontspan(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t blackPawnsFrontspan(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t whitePawnsRearspan(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t blackPawnsRearspan(uint64_t pawns_bb, uint64_t occupied_bb);

    double gamePhase();

    void whiteSpaceBonus();

    void blackSpaceBonus();

    void countOpenFiles();

    /** Determine score of position */

    void materialScore();

    void pawnStructure();

    void doubledPawns();

    void passedPawns();

    void backwardPawns();

    void rook_activity();

    void queen_activity();

    void king_placement();

    void evaluate_space();

    /** Helper functions below */

    static void
    accumulate_king_threats(int &n_attackers, int32_t &mg_score, int32_t &eg_score, uint64_t attacks, uint64_t king);

    void accumulate_psqts();

    void evaluate_psqt_w(uint64_t bb, const int32_t psqt[64], const int32_t eg_psqt[64]);

    void evaluate_psqt_b(uint64_t bb, const int32_t psqt[64], const int32_t eg_psqt[64]);

    void accumulate_characteristic_w(int n, Evaluation::characteristic_t type);

    void accumulate_characteristic_b(int n, Evaluation::characteristic_t type);
};