#include "Search.h"
#include "Piece.h"
#include "Evaluation.h"

/**
 * @brief Returns an integer value, representing the evaluation of the specified move.
 */

int32_t evaluate_position_max(uint16_t depth, int32_t alpha, int32_t beta);

int32_t evaluate_position_min(uint16_t depth, int32_t alpha, int32_t beta) {
    positions_seen.insert((uint8_t *) subject->position_hash);
    std::vector<uint32_t> *move_list = subject->generate_moves();
    if (move_list->size() == 0) {
        if (subject->is_king_in_check()) {
            return INT32_MAX;
        }
        return (int32_t) 0;
    }
    
    if (depth == 0) {
        return evaluate(subject);
    }
    int32_t value = INT32_MAX - 1;
    uint32_t best_move = 0;
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        subject->make_move(candidate_move);
        int32_t next_value = evaluate_position_max(depth - 1, alpha, beta);
        if (next_value < value) {
            value = next_value;
        }
        subject->revert_move();
        if (value <= alpha && value != INT32_MIN) {
            delete move_list;
            return value;
        }
        if (value < beta) {
            beta = value;
            best_move = candidate_move;
        }
    }
    delete move_list;
    top_line.push(best_move);
    return value;
}

int32_t evaluate_position_max(uint16_t depth, int32_t alpha, int32_t beta) {
    positions_seen.insert((uint8_t *) subject->position_hash);
    std::vector<uint32_t> *move_list = subject->generate_moves();
    if (move_list->size() == 0) {
        if (subject->is_king_in_check()) {
            return INT32_MIN;
        }
        return (int32_t) 0;
    }

    if (depth == 0) {
        return evaluate(subject);
    }
    int32_t value = INT32_MIN + 1;
    uint32_t best_move = 0;
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        subject->make_move(candidate_move);
        int32_t next_value = evaluate_position_min(depth - 1, alpha, beta);
        if (next_value > value) {
            value = next_value;
        }
        subject->revert_move();
        if (value >= beta && value != INT32_MAX) {
            delete move_list;
            return value;
        }
        if (value > alpha) {
            alpha = value;
            best_move = candidate_move;
        }
    }
    delete move_list;
    top_line.push(best_move);
    return value;
}


uint32_t search(Board *board, uint16_t depth) {
    subject = board;
    if  (board->move == Board::WHITE) {
        evaluate_position_max(depth, INT32_MIN + 1, INT32_MAX - 1);
    } else {
        evaluate_position_min(depth, INT32_MIN + 1, INT32_MAX - 1);
    }
    return top_line.top();
}