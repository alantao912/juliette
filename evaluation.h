#pragma once

#include <cstdint>
#include "util.h"

#define QUEEN_MATERIAL 900
#define ROOK_MATERIAL 500
#define BISHOP_MATERIAL 325
#define KNIGHT_MATERIAL 300
#define PAWN_MATERIAL 100

#define QUEEN_MATERIAL_E 900
#define ROOK_MATERIAL_E 500
#define BISHOP_MATERIAL_E 325
#define KNIGHT_MATERIAL_E 300
#define PAWN_MATERIAL_E 100

#define CONNECTED_PAWN_BONUS 5
#define RANK_5_PAWN_BONUS 50
#define RANK_6_PAWN_BONUS 150
#define RANK_7_PAWN_BONUS 250

#define DOUBLED_PAWN_PENALTY 20

#define CONNECTED_PAWN_BONUS_E 5
#define RANK_5_PAWN_BONUS_E 50
#define RANK_6_PAWN_BONUS_E 150
#define RANK_7_PAWN_BONUS_E 250

#define DOUBLED_PAWN_PENALTY_E 20

uint64_t reverse_bb(uint64_t bb);

int32_t evaluate();

void material_score();

void pawn_structure();

void pawn_progression();

void doubled_pawns();

void knight_activity();

void bishop_activity();

int32_t rook_activity();

int32_t queen_activity();