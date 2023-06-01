#pragma once

#include <cstdint>
#include "util.h"

struct Evaluation {
public:

    int32_t evaluate();

    void reset();

private:

    enum characteristic_t {
        PAWN_CHAIN, DOUBLED_PAWNS, CONNECTED_ROOKS, QUEEN_ROOK, QUEEN_BISHOP, KING_THREAT, PASSED_PAWN, BACKWARD_PAWN
    };

    uint64_t w_king_vulnerabilities, b_king_vulnerabilities;
    uint64_t w_pawn_rearspans, b_pawn_rearspans;

    int32_t n_open_files;
    int32_t w_space_bonus, b_space_bonus;
    int32_t midgame_score, endgame_score;

    int8_t square_guard_values[64];

    double progression;

    int32_t compute_score() const;

    /** Determine board characteristics */

    static uint64_t compute_king_vulnerabilities(uint64_t king_bb, uint64_t barriers);

    static uint64_t compute_pawn_frontspans_w(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t compute_pawn_frontspans_b(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t compute_pawn_rearspans_w(uint64_t pawns_bb, uint64_t occupied_bb);

    static uint64_t compute_pawn_rearspans_b(uint64_t pawns_bb, uint64_t occupied_bb);

    static double compute_progression();

    void compute_space_bonus_w();

    void compute_space_bonus_b();

    void compute_n_open_files();

    /** Determine score of position */

    void material_score();

    void pawn_structure();

    void doubled_pawns();

    void passed_pawns();

    void backward_pawns();

    void knight_activity();

    void bishop_activity();

    void rook_activity();

    void queen_activity();

    void king_placement();

    void evaluate_space();

    /** Helper functions below */

    void accumulate_psqts();

    void evaluate_psqt_w(uint64_t bb, const int32_t psqt[64], const int32_t eg_psqt[64]);

    void evaluate_psqt_b(uint64_t bb, const int32_t psqt[64], const int32_t eg_psqt[64]);

    void accumulate_characteristic_w(int n, Evaluation::characteristic_t type);

    void accumulate_characteristic_b(int n, Evaluation::characteristic_t type);
};