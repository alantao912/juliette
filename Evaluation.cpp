#include "Evaluation.h"
#include "Knight.h"

namespace MiddleGame {
    static short placement_incentives[][64] = {
        {
            0 ,  0,  0 ,   0,   0,   0,   0,   0,
            0 , 20, 30 ,  30,  30,  30,  20,   0,
            0 , 20, 40 ,  40,  40,  40,  20,   0,
            0 ,  0, 20 ,  20,  20,  20,   0,   0,
            -20, -10, 20 ,  20,  20,  20, -10, -20,
            -20, -20,  0 ,   0,   0,   0, -20, -20,
            -20, -20, -10, -10, -10, -10, -20, -20,
            -30, -20, -20, -20, -20, -20, -20, -30,
        },
        {
            // Bishop placement
        },
        {
            // Rook placement
        },
        {
            // Queen
        },
        {
            // King
        },
        {
            // Pawn
        }
    };
}

static int evaluate_material();

static short evaluate_placement();

static Board *evaluated_board = nullptr;

std::vector<Piece *> *my_pieces = nullptr;
std::vector<Piece *> *opponent_pieces = nullptr;

int evaluate(Board *board) {
    evaluated_board = board;

    if (evaluated_board->move == Board::WHITE) {
        my_pieces = evaluated_board->get_white_pieces();
        opponent_pieces = evaluated_board->get_black_pieces();
    } else {
        my_pieces = evaluated_board->get_black_pieces();
        opponent_pieces = evaluated_board->get_white_pieces();
    }

    int evaluation = 0;
    evaluation += evaluate_material();
    evaluation += evaluate_placement();
    return evaluation;
}

static int evaluate_material() {
    int material_evaluation = 0;

    for (Piece *p : *my_pieces) {
        if (p->is_taken) {
            continue;
        }
        char index = evaluated_board->offset(p->file, p->rank);
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
        if (p->is_taken) {
                continue;
        }
        char offset = evaluated_board->offset(p->file, p->rank);
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

static short evaluate_placement() {
    int placement_evaluation = 0;

    for (Piece *p : *my_pieces) {
        if (p->is_taken) {
            continue;
        }
        placement_evaluation += p->calculate_placement_value();
    }

    for (Piece *p : *opponent_pieces) {
        if (p->is_taken) {
            continue;
        }
        placement_evaluation -= p->calculate_placement_value();
    }
    return placement_evaluation;
}