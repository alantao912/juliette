#include <iostream>
#include <unordered_map>
#include "util.h"
#include "stack.h"
#include "tables.h"
#include "search.h"
#include "movegen.h"
#include "bitboard.h"
#include "evaluation.h"

#define MIN_SCORE (INT32_MIN + 100)
#define CHECKMATE(depth) ((MIN_SCORE + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) 0

std::unordered_map<uint64_t, TTEntry> transposition_table;
std::unordered_map<uint64_t, RTEntry> repetition_table;

extern bitboard board;
std::vector<move_t> top_line;

/**
 * @brief Extends the search position until a "quiet" position is reached.
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 * @return
 */

int32_t quiescence_search(int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
    std::cout << "qsearch " << std::endl;
    move_t moves[MAX_MOVE_NUM];
    int n;
    if (!is_check(board.turn)) {
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        n = gen_nonquiescent_moves(moves, board.turn);
        if (!n) {
            /** Position is quiet, return evaluation. */
            return evaluate();
        }
    } else if (!(n = gen_legal_moves(moves, board.turn))) {
        /** Checkmate :( */
        return CHECKMATE(0);
    }
    std::vector<move_t> subsequent_lines[n];
    size_t best_move_index = 0;
    int32_t value = INT32_MIN;
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = -quiescence_search(-beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (next_value > value) {
            best_move_index = i;
            value = next_value;
        }
        if (value >= beta) {
            /** Prune */
            goto PRUNE;
        }
        if (value > alpha) {
            alpha = value;
        }
    }
    for (move_t m : subsequent_lines[best_move_index]) {
        considered_line->push_back(m);
    }
    PRUNE:
    std::cout << "Returning" << std::endl;
    return value;
}

/**
 * @brief Returns an integer value, representing the evaluation of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

int32_t negamax(uint16_t remaining_depth, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
    uint64_t hash_code = board.zobrist;
    auto t = transposition_table.find(hash_code);
    if (t != transposition_table.end()) {
        /** Current board state has been reached before. */
        TTEntry &entry = t->second;
        if (!entry.searched || entry.depth >= remaining_depth) {
            return entry.evaluation;
        }
        /** Continue normal search if current position has been searched before and was searched at a lower remaining_depth */
        entry.searched = false;
        entry.evaluation = alpha;
        entry.depth = remaining_depth;
    } else {
        /** Novel position has been reached */
        transposition_table.insert(std::pair<uint64_t, TTEntry>(hash_code, TTEntry(alpha, remaining_depth)));
    }
    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    if (!n) {
        TTEntry &entry = transposition_table.find(hash_code)->second;
        entry.searched = true;
        if (is_check(board.turn)) {
            /** King is in check, and there are no legal moves. Checkmate */
            entry.evaluation = CHECKMATE(remaining_depth);
            return entry.evaluation;
        }
        /** No legal moves, yet king is not in check. This is a stalemate, and the game is drawn. */
        entry.evaluation = DRAW;
        return entry.evaluation;
    }
    if (!remaining_depth) {
        /** Extend the search until the position is quiet */
        TTEntry &entry = transposition_table.find(hash_code)->second;
        entry.searched = true;
        entry.evaluation = quiescence_search(alpha, beta, considered_line);
        return entry.evaluation;
    }
    int32_t value = INT32_MIN;
    size_t best_move_index = 0;
    std::vector<move_t> subsequent_lines[n];
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = -negamax(remaining_depth - 1, -beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (next_value > value) {
            value = next_value;
            best_move_index = i;
        }
        if (value >= beta) {
            goto PRUNE;
        }
        if (value > alpha) {
            alpha = value;
        }
    }
    for (move_t m : subsequent_lines[best_move_index]) {
        considered_line->push_back(m);
    }
    PRUNE:
    /** Current node has been fully searched, update evaluation. */
    auto entry = transposition_table.find(hash_code);
    entry->second.searched = true;
    entry->second.evaluation = value;
    return value;
}

move_t search(uint16_t depth) {
    top_line.clear();
    negamax(depth, MIN_SCORE, -MIN_SCORE, &top_line);
    return top_line.front();
}

void showTopLine() {
    std::cout << "Top line: ";
    for (size_t i = 0; i < top_line.size() - 1; ++i) {
        move_t move = top_line.at(i);
        print_move(move);
        std::cout << ", ";
    }
    if (!top_line.empty()) {
        print_move(top_line.back());
    }
    std::cout << '\n';
}