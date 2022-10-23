#include "Evaluation.h"

/* Pointer to board object that we are currently evaluating */
extern bitboard board;

int32_t evaluate() {
    int evaluation = 0;
    evaluation += material_score();
    return (1 - 2 * (board.turn == BLACK)) * evaluation;
}

int32_t material_score() {
    int material_evaluation = 0;
    return material_evaluation;
}