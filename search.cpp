#include <iostream>
#include <unordered_map>
#include <cstring>
#include "util.h"
#include "stack.h"
#include "tables.h"
#include "search.h"
#include "movegen.h"
#include "bitboard.h"
#include "evaluation.h"

/**
 * Quiescent search max ply count.
 */

const uint16_t qsearch_lim = 4;

/**
 * Contempt factor indicates respect for opponent.
 * Positive contempt indicates respect for stronger opponent.
 * Negative contempt indicates perceived weaker opponent
 */
const int32_t contempt = 0;

#define MIN_SCORE (INT32_MIN + 100)
#define CHECKMATE(depth) ((MIN_SCORE + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) contempt;


std::unordered_map<uint64_t, TTEntry> transposition_table;
std::unordered_map<uint64_t, RTEntry> repetition_table;

extern bitboard board;
extern stack_t *stack;
std::vector<move_t> top_line;

static inline bool is_drawn() {
    auto iterator = repetition_table.find(board.hash_code);
    if (iterator != repetition_table.end()) {
        const RTEntry &rt_entry = iterator->second;
        if (rt_entry.num_seen >= 3) {
            return verify_repetition(iterator->first);
        }
    }
    return board.fullmove_number >= 50;
}

static inline bool verify_repetition(uint64_t hash) {
    uint8_t num_seen = 0;
    stack_t *iterator = stack;
    while (iterator) {
        if (iterator->board.hash_code == hash && strncmp(reinterpret_cast<const char *>(&(iterator->board)),
                                                         reinterpret_cast<const char *>(&board), sizeof(bitboard)) == 0) {
            ++num_seen;
            if (num_seen >= 3) {
                return true;
            }
        }
        iterator = iterator->next;
    }
    return false;
}

/**
 * @brief Extends the search position until a "quiet" position is reached.
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 * @return
 */

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
    if (is_drawn()) {
        return DRAW;
    }
    move_t moves[MAX_MOVE_NUM];
    int n;
    if (!is_check(board.turn)) {
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        if (!remaining_ply || !(n = gen_nonquiescent_moves(moves, board.turn))) {
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
        if (alpha < stand_pat) {
            alpha = stand_pat;
        }
    }
    CHECK_EVASIONS:
    ssize_t best_move_index = -1;
    std::vector<move_t> subsequent_lines[n];
    for (ssize_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t score = -quiescence_search(remaining_ply, -beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_index = i;
        }
    }
    if (best_move_index != -1) {
        for (move_t m: subsequent_lines[best_move_index]) {
            considered_line->push_back(m);
        }
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
    const int32_t original_alpha = alpha;
    auto t = transposition_table.find(board.hash_code);
    if (t != transposition_table.end() && t->second.depth >= remaining_ply) {
        const TTEntry &tt_entry = t->second;
        switch (tt_entry.flag) {
            case EXACT:
                return tt_entry.evaluation;
            case LOWER:
                alpha = std::max(alpha, tt_entry.evaluation);
                break;
            case UPPER:
                beta = std::min(beta, tt_entry.evaluation);
                break;
        }
    }
    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    if (!n) {
        if (is_check(board.turn)) {
            /** King is in check, and there are no legal moves. Checkmate */
            return CHECKMATE(remaining_ply);
        }
        /** No legal moves, yet king is not in check. This is a stalemate, and the game is drawn. */
        return DRAW;
    }
    if (is_drawn()) {
        return DRAW;
    }
    if (!remaining_ply) {
        /** Extend the search until the position is quiet */
        return quiescence_search(qsearch_lim, alpha, beta, considered_line);
    }
    int32_t value = MIN_SCORE;
    size_t best_move_index = 0;
    std::vector<move_t> subsequent_lines[n];
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t sub_score = -negamax(remaining_ply - 1, -beta, -alpha, &(subsequent_lines[i]));
        pop();
        if (sub_score > value) {
            value = sub_score;
            best_move_index = i;
        }
        if (value > alpha) {
            alpha = value;
        }
        if (alpha >= beta) {
            goto PRUNE;
        }
    }
    /** Propogates up the principal variation */
    for (move_t m : subsequent_lines[best_move_index]) {
        considered_line->push_back(m);
    }
    PRUNE:
    /** Updates the transposition table with the appropriate values */
    TTEntry tt_entry(0, 0, EXACT);
    if (value <= original_alpha) {
        tt_entry.flag = UPPER;
    } else if (value >= beta) {
        tt_entry.flag = LOWER;
    }
    tt_entry.depth = remaining_ply;
    if (t != transposition_table.end()) {
        t->second = tt_entry;
    } else {
        transposition_table.insert(std::pair<uint64_t, TTEntry>(board.hash_code, tt_entry));
    }
    return value;
}

move_t search(uint16_t depth) {
    top_line.clear();
    int32_t evaluation = negamax(depth, MIN_SCORE, -MIN_SCORE, &top_line);
    std::cout << "Evaluation: " << evaluation << std::endl;
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