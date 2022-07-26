#include "Evaluation.h"

#include <iostream>

#define NUM_EVALUATORS 1
static int (*evaluation_functions[NUM_EVALUATORS]) (Board *);

static int evaluate_material(Board *board);

void initialize_evaluators() {
    evaluation_functions[0] = &evaluate_material;
}

int evaluate(Board *board) {
    return evaluate_material(board);
}

static int evaluate_material(Board *board) {
    int material_evaluation = 0;

    std::vector<Piece *> *my_pieces = nullptr;
    std::vector<Piece *> *opponent_pieces = nullptr;

    if (board->move == Board::WHITE) {
        my_pieces = board->get_white_pieces();
        opponent_pieces = board->get_black_pieces();
    } else {
        my_pieces = board->get_black_pieces();
        opponent_pieces = board->get_white_pieces();
    }

    for (Piece *p : *my_pieces) {
        if (p->is_taken) {
            std::cout << "Found taken piece in own pieces" << std::endl;
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

    for (Piece *p : *opponent_pieces) {
         switch (p->get_type()) {
            if (p->is_taken) {
                std::cout << "Found taken piece in opponent pieces!" << std::endl;
                continue;
            }
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
    std::cout << material_evaluation << std::endl;
    return material_evaluation;
}