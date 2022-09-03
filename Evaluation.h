#pragma once

#include "Board.h"
#include "Piece.h"

#define QUEEN_MATERIAL 900
#define ROOK_MATERIAL 500
#define BISHOP_MATERIAL 325
#define KNIGHT_MATERIAL 300
#define PAWN_MATERIAL 100

void initialize_evaluation();

int32_t evaluate();

int32_t material_score();