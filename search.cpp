#include <iostream>
#include <unordered_map>
#include "util.h"
#include "stack.h"
#include "tables.h"
#include "search.h"
#include "movegen.h"
#include "bitboard.h"
#include "evaluation.h"

const uint16_t qsearch_lim = 6;
const int32_t contempt = 0;

#define MIN_SCORE (INT32_MIN + 100)
#define CHECKMATE(depth) ((MIN_SCORE + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) -contempt;


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

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
    if (repetition_table.find(board.zobrist)->second.num_seen >= 3 || board.fullmove_number >= 50) {
        return DRAW;
    }
    move_t moves[MAX_MOVE_NUM];
    int n;
    if (!is_check(board.turn)) {
        /** Has reached qsearch limit */
        if (!remaining_ply) {
            return evaluate();
        }
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        n = gen_nonquiescent_moves(moves, board.turn);
        if (!n) {
            /** Position is quiet, return evaluation. */
            return evaluate();
        }
    } else if ((n = gen_legal_moves(moves, board.turn))) {
        /** Side to move is in check, evasions exist. */
        goto CHECK_EVASIONS;
    } else {
        /** Side to move is in check, evasions do not exist. Checkmate :( */
        return CHECKMATE(remaining_ply);
    }

    {
        --remaining_ply;
        int32_t stand_pat = evaluate();
        if (stand_pat >= beta) {
            return beta;
        }
        if (stand_pat > alpha) {
            alpha = stand_pat;
        }
    }

    CHECK_EVASIONS:
    std::vector<move_t> subsequent_lines[n];
    size_t best_move_index = 0;
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t value = -quiescence_search(remaining_ply, -beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (value >= beta) {
            return beta;
        }
        if (value > alpha) {
            alpha = value;
        }
    }
    for (move_t m : subsequent_lines[best_move_index]) {
        considered_line->push_back(m);
    }
    return alpha;
}

/**
 * @brief Returns an integer value, representing the evaluation of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

int32_t negamax(uint16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
    uint64_t hash_code = board.zobrist;
    /** Implements draw by three-fold repetition and 50 move draw rule. */
    if (repetition_table.find(hash_code)->second.num_seen >= 3 || board.fullmove_number >= 50) {
        return DRAW;
    }
    auto t = transposition_table.find(hash_code);
    if (t != transposition_table.end()) {
        /** Current board state has been reached before. */
        TTEntry &entry = t->second;
        if (!entry.searched || entry.depth >= remaining_ply) {
            return entry.evaluation;
        }
        /** Continue normal search if current position has been searched before and was searched at a lower remaining_ply */
        entry.searched = false;
        entry.evaluation = alpha;
        entry.depth = remaining_ply;
    } else {
        /** Novel position has been reached */
        transposition_table.insert(std::pair<uint64_t, TTEntry>(hash_code, TTEntry(alpha, remaining_ply)));
    }
    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    if (!n) {
        TTEntry &entry = transposition_table.find(hash_code)->second;
        entry.searched = true;
        if (is_check(board.turn)) {
            /** King is in check, and there are no legal moves. Checkmate */
            entry.evaluation = CHECKMATE(remaining_ply);
            return entry.evaluation;
        }
        /** No legal moves, yet king is not in check. This is a stalemate, and the game is drawn. */
        entry.evaluation = DRAW;
        return entry.evaluation;
    }
    if (!remaining_ply) {
        /** Extend the search until the position is quiet */
        TTEntry &entry = transposition_table.find(hash_code)->second;
        entry.evaluation = quiescence_search(qsearch_lim, alpha, beta, considered_line);
        entry.searched = true;
        return entry.evaluation;
    }
    int32_t value = INT32_MIN;
    size_t best_move_index = 0;
    std::vector<move_t> subsequent_lines[n];
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t next_value = -negamax(remaining_ply - 1, -beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (next_value > value) {
            value = next_value;
            best_move_index = i;
        }
        if (value > alpha) {
            alpha = value;
        }
        if (alpha >= beta) {
            goto PRUNE;
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