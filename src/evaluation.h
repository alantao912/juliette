#pragma once

#include <cstdint>
#include "util.h"

struct Evaluation {
public:
    int32_t evaluate();

private:

    enum characteristic_t {
        PAWN_CHAIN, DOUBLED_PAWNS, CONNECTED_ROOKS, QUEEN_ROOK, QUEEN_BISHOP, KING_THREAT
    };

    uint64_t w_king_vulnerabilities, b_king_vulnerabilities;
    int32_t midgame_score, endgame_score;

    double progression;

    void reset();

    int32_t compute_score() const;

    static uint64_t compute_king_vulnerabilities(uint64_t king_bb, uint64_t pawns_bb);

    static double compute_progression();

    void material_score();

    void pawn_structure();

    void doubled_pawns();

    void knight_activity();

    void bishop_activity();

    void rook_activity();

    void queen_activity();

    void king_safety();

    void king_mobility();

    void passed_pawns();
};