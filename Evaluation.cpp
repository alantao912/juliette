#include "Evaluation.h"
#include "Knight.h"

namespace PlacementIncentives {
    static const short knight[64] = {
    -30, -20, -20, -20, -20, -20, -20, -30,
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,  0 ,   0,   0,   0,   0, -10,
    -10,  0, 10 ,  10,  10,  10,   0, -10,
    0 ,  0,  20 ,  20,  20,  20,   0,   0,
    0 , 20,  40 ,  40,  40,  40,  20,   0,
    0 , 20,  30 ,  30,  30,  30,  20,   0,
    0 ,  0,   0 ,   0,   0,   0,   0,   0,
    };

    static const short pawn[64] {
    // index 0 is a1
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     5,  7, 10, 10, 10, 10,  7,  5,
    10, 15, 20, 20, 20, 20, 15, 10,
    12, 15, 15, 20, 20, 15, 15, 12,
    15, 20, 20, 25, 25, 20, 20, 15,
     0,  0,  0,  0,  0,  0,  0,  0,
                                    // index 63 is h8
    };
};

static short dereference_hitboard(uint64_t hit_board, const short *incentives,  uint8_t (Board::*indexing_function) (uint8_t, uint8_t)) {
    short e = 0;
    uint8_t i = 0;
    while (hit_board) {
        if (hit_board & 1) {
            e += incentives[(subject->*indexing_function) (i % 8, i / 8)];
        }
        hit_board >>= 1;
    }
    return e;
}

static int material_score();

static short piece_placement_score(Board::Color color, uint8_t (Board::*indexing_function) (uint8_t, uint8_t));

/* Pointer to board object that we are currently evaluating */
static Board *subject = nullptr;

/* Pieces of the current turn color. */
std::vector<Piece *> *current_pieces = nullptr;

/* Pieces of the opposite turn color. */
std::vector<Piece *> *opponent_pieces = nullptr;

int evaluate(Board *board) {
    subject = board;
    current_pieces = board->get_pieces_of_color(board->move);
    opponent_pieces = board->get_opposite_pieces(board->move);

    int evaluation = 0;
    evaluation += material_score();

    uint8_t (Board::*current_indexing_function) (uint8_t, uint8_t) = Board::offset;
    uint8_t (Board::*opponent_indexing_function) (uint8_t, uint8_t) = Board::offset_invert_rank;
    if (subject->move == Board::BLACK) {
        current_indexing_function = Board::offset_invert_rank;
        opponent_indexing_function = Board::offset;
    }
    
    evaluation += (piece_placement_score(subject->move, current_indexing_function) - piece_placement_score((Board::Color) ((subject->move + 1) % 2), opponent_indexing_function));
    return evaluation;
}

static int material_score() {
    int material_evaluation = 0;

    for (Piece *p : *current_pieces) {
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

    for (Piece *p : *opponent_pieces) {
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

static short piece_placement_score(Board::Color color, uint8_t (Board::*indexing_function) (uint8_t, uint8_t)) {
    short placement_evaluation = 0;
    /* Collection of pieces of the same type */
    std::vector<Piece *> *piece_collection = nullptr;
    /* 64-bit bitboard which indicate the squares hit by each of the collective piece types. */
    uint64_t hitboard = (uint64_t) 0;
    piece_collection = subject->get_collection(color, KNIGHT);
    /* Computes bitboard for all squares hit by knights */
    for (size_t i = 0; i < piece_collection->size(); ++i) {
        Piece *knight = piece_collection->at(i);
        hitboard |= knight->squares_hit;
    }
    placement_evaluation += dereference_hitboard(hitboard, PlacementIncentives::knight, indexing_function);

    hitboard = (uint64_t) 0;
    piece_collection = subject->get_collection(color, PAWN);
    for (size_t i = 0; i < piece_collection->size(); ++i) {
        Piece *pawn = piece_collection->at(i);
        hitboard |= pawn->squares_hit;
    }
    placement_evaluation += dereference_hitboard(hitboard, PlacementIncentives::pawn, indexing_function);
    return placement_evaluation;
}