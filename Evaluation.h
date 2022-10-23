#pragma once

#include <stdint.h>
#include "util.h"

#define QUEEN_MATERIAL 900
#define ROOK_MATERIAL 500
#define BISHOP_MATERIAL 325
#define KNIGHT_MATERIAL 300
#define PAWN_MATERIAL 100

int32_t evaluate();

int32_t material_score();