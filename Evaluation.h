#pragma once

#include "Board.h"
#include "Piece.h"

#define QUEEN_MATERIAL 900
#define ROOK_MATERIAL 500
#define BISHOP_MATERIAL 325
#define KNIGHT_MATERIAL 300
#define PAWN_MATERIAL 100

/* Pointer to board object that we are currently evaluating */
static Board *b = nullptr;

static std::vector<Piece *> *white_pieces = nullptr;
static std::vector<Piece *> *black_pieces = nullptr;

void initialize_evaluation(Board *board);

int32_t evaluate();

int32_t material_score();