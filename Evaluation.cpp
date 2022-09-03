#include "Evaluation.h"

/* Pointer to board object that we are currently evaluating */
extern Board *game;

std::vector<Piece *> *white_pieces = nullptr;
std::vector<Piece *> *black_pieces = nullptr;

void initialize_evaluation() {
    white_pieces = game->get_white_pieces();
    black_pieces = game->get_black_pieces();
}

int32_t evaluate() {
    int evaluation = 0;
    evaluation += material_score();
    return evaluation;
}

int32_t material_score() {
    int material_evaluation = 0;

    for (Piece *p : *white_pieces) {
        if (p->is_taken) {
            continue;
        }
        switch (p->get_type()) {
            case QUEEN:
                material_evaluation += QUEEN_MATERIAL;
            break;
            case ROOK:
                material_evaluation += ROOK_MATERIAL;
            break;
            case BISHOP:
                material_evaluation += BISHOP_MATERIAL;
            break;
            case KNIGHT:
                material_evaluation += KNIGHT_MATERIAL;
            break;
            case PAWN:
                material_evaluation += PAWN_MATERIAL;
            break;
        }
    }

    for (Piece *p : *black_pieces) {
        if (p->is_taken) {
                continue;
        }
        switch (p->get_type()) {
            case QUEEN:
                material_evaluation -= QUEEN_MATERIAL;
            break;
            case ROOK:
                material_evaluation -= ROOK_MATERIAL;
            break;
            case BISHOP:
                material_evaluation -= BISHOP_MATERIAL;
            break;
            case KNIGHT:
                material_evaluation -= KNIGHT_MATERIAL;
            break;
            case PAWN:
                material_evaluation -= PAWN_MATERIAL;
            break;

        }
    }
    return material_evaluation;
}