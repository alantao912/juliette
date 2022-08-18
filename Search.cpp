#include "Search.h"
#include "Evaluation.h"

/**
 * @brief Returns an integer value, representing the evaluation of the specified move.
 */

int32_t evaluate_position(uint16_t depth, int32_t alpha, int32_t beta) {
    positions_seen.insert((uint8_t *) subject->position_hash);
    if (depth == 0) {
        return evaluate(subject);
    }
    uint32_t best_move = 0;
    std::vector<uint32_t> *move_list = subject->generate_moves();
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        subject->make_move(candidate_move);
        evaluate_position(depth - 1, alpha, beta);
        subject->revert_move();
    }
    delete move_list;
    top_line.push(best_move);
    return 0;
}


uint32_t search(Board *board, uint16_t depth) {
    subject = board;
    evaluate_position(depth, INT32_MIN, INT32_MIN);
    return top_line.top();
}