#include <iostream>
#include "evaluation.h"

/* Pointer to board object that we are currently evaluating */
extern bitboard board;

int32_t evaluate() {
    int evaluation = 0;
    evaluation += material_score();
    return (1 - 2 * (board.turn == BLACK)) * evaluation;
}

int32_t material_score() {
    int material_evaluation = 0;
    material_evaluation += pop_count(board.w_pawns) * PAWN_MATERIAL;
    material_evaluation += pop_count(board.w_knights) * KNIGHT_MATERIAL;
    material_evaluation += pop_count(board.w_bishops) * BISHOP_MATERIAL;
    material_evaluation += pop_count(board.w_rooks) * ROOK_MATERIAL;
    material_evaluation += pop_count(board.w_queens) * QUEEN_MATERIAL;
    material_evaluation -= pop_count(board.b_pawns) * PAWN_MATERIAL;
    material_evaluation -= pop_count(board.b_knights) * KNIGHT_MATERIAL;
    material_evaluation -= pop_count(board.b_bishops) * BISHOP_MATERIAL;
    material_evaluation -= pop_count(board.b_rooks) * ROOK_MATERIAL;
    material_evaluation -= pop_count(board.b_queens) * QUEEN_MATERIAL;
    return material_evaluation;
}