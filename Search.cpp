#include <unordered_map>
#include "Zobrist.h"
#include "Search.h"
#include "Evaluation.h"

#define CHECKMATE(depth) ((INT32_MIN + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) 0;

std::unordered_map<uint64_t, TTEntry> transposition_table;

extern Board *game;
std::vector<uint32_t> top_line;

void initialize_search() {
    top_line.clear();
    transposition_table.clear();
    initialize_zobrist();
}

/**
 * @brief Returns an integer value, representing the evaluation of the specified turn.
 * 
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

int32_t negamax(uint16_t depth, int32_t alpha, int32_t beta, std::vector<uint32_t> *considered_line) {
    uint64_t hash_code = hash(game);
    std::unordered_map<uint64_t, TTEntry>::const_iterator t;
    if ((t = transposition_table.find(hash_code)) != transposition_table.end()) {
        TTEntry entry = t->second;
    }
    std::vector<uint32_t> *move_list = game->generate_moves();
    if (move_list->empty()) {
        if (game->is_king_in_check()) {
            /* Return negative or positive infinity when white to play and black to place respectively. */
            return CHECKMATE(depth);
        }
        /* Stalemate. The game is drawn. */
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
        game->make_move(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = -negamax(depth - 1, -beta, -alpha, &(subsequent_lines[i]));
        game->revert_move();
        if (next_value > value) {
            value = next_value;
            best_move_index = i;
        } else {
            // TODO Free vector of inferior line
        }
        if (value >= beta) {
            /* Beta cutoff. Previous turn is refuted. No further moves need be considered */
            /* TODO: Add move to killer heuristic */
            goto PRUNE;
        }
        if (value > alpha) {
            alpha = value;
        }
    }
    for (unsigned int & i : subsequent_lines[best_move_index]) {
        considered_line->push_back(i);
    }
    PRUNE:
    delete move_list;
    /* Current node has been searched */
    transposition_table.insert(std::pair<uint64_t, TTEntry>(hash_code, TTEntry(value, depth, 0)));
    return value;
}

uint32_t search(uint16_t depth) {
    negamax(depth, INT32_MIN, INT32_MAX, &top_line);
    return top_line.front();
}

void showTopLine() {
    for (size_t i = 0; i < top_line.size() - 1; ++i) {
        uint32_t move = top_line.at(i);
        Board::print_move(move);
        std::cout << ", ";
    }
    Board::print_move(top_line.back());
}