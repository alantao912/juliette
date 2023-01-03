#pragma once

#include <cstdint>
#include "util.h"

typedef struct eval_stats {
    double progression;
    int32_t midgame_score, endgame_score;

    uint64_t w_king_vulnerabilities, b_king_vulnerabilities;

    void reset();

    int32_t compute_score();
private:
    double compute_progression();
    uint64_t compute_king_vulnerabilities(uint64_t king, uint64_t pawns);
} eval_stats;

uint64_t reverse_bb(uint64_t bb);


int32_t evaluate();

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