#include "Search.h"
#include "Piece.h"
#include "Evaluation.h"

#define BLACK_CHECKMATE(depth) ((INT32_MAX - 1) - (UINT16_MAX - depth))
#define WHITE_CHECKMATE(depth) ((INT32_MIN + 1) + (UINT16_MAX - depth))
#define DRAW (uint32_t) 0;

inline bool contains_checks_or_captures(std::vector<uint32_t> *move_list) {
    uint32_t move = move_list->at(0);
    return !(GET_IS_CAPTURE(move) || !GET_IS_CHECK(move));
}

/**
 * @brief Returns an integer value, representing the evaluation of the specified move.
 * 
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

int32_t evaluate_position_max(uint16_t depth, int32_t alpha, int32_t beta, std::vector<uint32_t> *considered_line);

int32_t evaluate_position_min(uint16_t depth, int32_t alpha, int32_t beta, std::vector<uint32_t> *considered_line) {
    positions_seen.insert((uint8_t *) subject->position_hash);
    std::vector<uint32_t> *move_list = subject->generate_moves();
    if (move_list->size() == 0) {
        if (subject->is_king_in_check()) {
            return BLACK_CHECKMATE(depth);
        }
        return DRAW;
    }
    
    if (depth == 0) {
        return evaluate();
    }
    int32_t value = INT32_MAX;
    size_t best_move_index = 0;
    std::vector<uint32_t> subsequent_lines[move_list->size()];
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        subject->make_move(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = evaluate_position_max(depth - 1, alpha, beta, &(subsequent_lines[i]));
        if (next_value < value) {
            /* candidate_move is better than existing moves searched. */
            value = next_value;
        } else {
            // TODO: Free vector of inferior line.
        }
        subject->revert_move();
        if (value < alpha) {
            /* alpha cutoff. Previous move has been refuted. No further moves need be considered. */
            delete move_list;
            return value;
        }
        if (value < beta) {
            beta = value;
            best_move_index = i;
        }
    }
    for (size_t i = 0; i < subsequent_lines[best_move_index].size(); ++i) {
        considered_line->push_back(subsequent_lines[best_move_index].at(i));
    }
    delete move_list;
    return value;
}

int32_t evaluate_position_max(uint16_t depth, int32_t alpha, int32_t beta, std::vector<uint32_t> *considered_line) {
    positions_seen.insert((uint8_t *) subject->position_hash);
    std::vector<uint32_t> *move_list = subject->generate_moves();
    if (move_list->size() == 0) {
        if (subject->is_king_in_check()) {
            return WHITE_CHECKMATE(depth);
        }
        return DRAW;
    }

    if (depth == 0) {
        return evaluate();
    }
    int32_t value = INT32_MIN;
    size_t best_move_index = 0;
    std::vector<uint32_t> subsequent_lines[move_list->size()];
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        subject->make_move(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = evaluate_position_min(depth - 1, alpha, beta, &(subsequent_lines[i]));
        if (next_value > value) {
            value = next_value;
        } else {
            // Free vector of inferior line.
        }
        subject->revert_move();
        if (value >= beta) {
            /* Beta cutoff. Previous move is refuted. No further moves need be considered */
            delete move_list;
            return value;
        }
        if (value > alpha) {
            alpha = value;
            best_move_index = i;
        }
    }
    for (size_t i = 0; i < subsequent_lines[best_move_index].size(); ++i) {
        considered_line->push_back(subsequent_lines[best_move_index].at(i));
    }
    delete move_list;
    return value;
}

void initialize_search(Board *board) {
    subject = board;
}


uint32_t search(uint16_t depth) {
    if  (subject->move == Board::WHITE) {
        evaluate_position_max(depth, INT32_MIN, INT32_MAX, &top_line);
    } else {
        evaluate_position_min(depth, INT32_MIN, INT32_MAX, &top_line);
    }
    return top_line.front();
}

void show_top_line() {
    std::cout << "Top line: ";
    for (size_t i = 0; i < top_line.size() - 1; ++i) {
        uint32_t move = top_line.at(i);
        subject->print_move(move);
        std::cout << ", ";
        subject->make_move(move);
    }
    subject->print_move(top_line.back());
    subject->make_move(top_line.back());

    for (size_t i = 0; i < top_line.size(); ++i) {
        subject->revert_move();
    }
}