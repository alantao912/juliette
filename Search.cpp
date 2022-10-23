#include <iostream>
#include <unordered_map>
#include "tables.h"
#include "Search.h"
#include "Evaluation.h"
#include "bitboard.h"
#include "movegen.h"
#include "util.h"
#include "stack.h"

#define CHECKMATE(depth) ((INT32_MIN + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) 0

std::unordered_map<uint64_t, TTEntry> transposition_table;

extern bitboard board;
std::vector<Move> top_line;

/**
 * @brief Returns an integer value, representing the evaluation of the specified turn.
 * 
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

int32_t negamax(uint16_t depth, int32_t alpha, int32_t beta, std::vector<Move> *considered_line) {
    uint64_t hash_code = board.zobrist;
    std::unordered_map<uint64_t, TTEntry>::iterator t;
    uint8_t num_seen = 0;
    if ((t = transposition_table.find(hash_code)) != transposition_table.end()) {
        TTEntry entry = t->second;
        if (entry.num_seen >= 3) {
            return DRAW;
        }
        if (entry.depth >= depth) {
            return entry.evaluation;
        }
        ++entry.num_seen;
        num_seen = entry.num_seen;
    }
    Move moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    if (n == 0) {
        if (is_check(board.turn)) {
            /* Return negative or positive infinity when white to play and black to place respectively. */
            return CHECKMATE(depth);
        }
        /* Stalemate. The board is drawn. */
        return DRAW;
    }
    if (depth == 0) {
        return evaluate();
    }
    int32_t value = INT32_MIN;
    size_t best_move_index = 0;
    std::vector<Move> subsequent_lines[n];
    for (size_t i = 0; i < n; ++i) {
        Move candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = -negamax(depth - 1, -beta, -alpha, &(subsequent_lines[i]));
        pop();
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
    for (Move i : subsequent_lines[best_move_index]) {
        considered_line->push_back(i);
    }
    PRUNE:
    /* Current node has been searched */
    transposition_table.insert(std::pair<uint64_t, TTEntry>(hash_code, TTEntry(value, depth, num_seen)));
    return value;
}

Move search(uint16_t depth) {
    negamax(depth, INT32_MIN, INT32_MAX, &top_line);
    return top_line.front();
}

void showTopLine() {
    for (size_t i = 0; i < top_line.size() - 1; ++i) {
        Move move = top_line.at(i);
        print_move(move);
        std::cout << ", ";
    }
    print_move(top_line.back());
}