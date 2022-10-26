#pragma once

#include <cstdint>
#include "util.h"

uint64_t reverse_bb(uint64_t bb);

double game_progression();

int32_t evaluate();

void material_score();

void pawn_structure();

void pawn_progression();

void doubled_pawns();

void knight_activity();

void bishop_activity();

void rook_activity();

void queen_activity();