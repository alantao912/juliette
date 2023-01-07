#include <cstring>
#include <algorithm>
#include <unordered_map>
#include "util.h"
#include "stack.h"
#include "tables.h"
#include "search.h"
#include "weights.h"
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
        if (iterator->board.hash_code == hash) {

            /**
             * When checking if two positions are equal, it could be the case that two positions are equal, but have
             * different half move and full move number values. Thus, before and after using strncmp, we save and
             * restore the true values of the two varaibles.
             */

            int t0 = iterator->board.halfmove_clock;
            int t1 = iterator->board.fullmove_number;
            iterator->board.halfmove_clock = board.halfmove_clock;
            iterator->board.fullmove_number = board.fullmove_number;
            if (strncmp(reinterpret_cast<const char *>(&(iterator->board)),
                        reinterpret_cast<const char *>(&board), sizeof(bitboard)) == 0) {
                ++num_seen;
            }
            iterator->board.halfmove_clock = t0;
            iterator->board.fullmove_number = t1;
            if (num_seen >= 3) {
                return true;
            }
        }
        iterator = iterator->next;
    }
    return false;
}

static inline bool contains_promotions() {
    uint64_t proms;
    if (board.turn) {
        /** Checks if white has any pawn promotions */
        proms = ((board.w_pawns & BB_RANK_7) << 8) & ~board.occupied;
        proms |= ((((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H)) & BB_RANK_8) & board.b_occupied;
    } else {
        /** Checks if black has any pawn promotions */
        proms = ((board.b_pawns & BB_RANK_2) >> 8) & ~board.occupied;
        proms |= ((((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A)) & BB_RANK_1) & board.w_occupied;
    }
    return pop_count(proms) > 0;
}

/**
 * @brief Extends the search position until a "quiet" position is reached.
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 * @return
 */

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta) {
    if (is_drawn()) {
        return DRAW;
    }

    int32_t stand_pat = MIN_SCORE;

    move_t moves[MAX_MOVE_NUM];
    int n, n_checks;
    if (!is_check(board.turn)) {
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        if (!remaining_ply || !(n = gen_nonquiescent_moves(moves, board.turn, &n_checks))) {
            /** Position is quiet, return evaluation. */
            return evaluate();
        }
    } else if ((n = gen_legal_moves(moves, board.turn))) {
        /** Side to move is in check, evasions exist. */
        /** Effectively, this line disables futility pruning. No futility pruning in check. */
        n_checks = n;
        goto CHECK_EVASIONS;
    } else {
        /** Side to move is in check, evasions do not exist. Checkmate :( */
        return CHECKMATE(remaining_ply);
    }

    --remaining_ply;
    stand_pat = evaluate();
    if (stand_pat >= beta) {
        return beta;
    }

    {
        int big_delta = QUEEN_MATERIAL;
        if (contains_promotions()) {
            big_delta += 775;
        }
        /** No move can possibly raise alpha. Prune this node. */
        if (stand_pat < alpha - big_delta) {
            /** https://www.chessprogramming.org/Delta_Pruning advises to return alpha.
             *  However, we must still check moves that give check. */
             n = n_checks;
        }
    }

    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    CHECK_EVASIONS:
    for (ssize_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        if (i >= n_checks && value(candidate_move) + stand_pat < alpha - DELTA_MARGIN) {
            /** Skip evaluating this move */
            continue;
        }
        push(candidate_move);
        int32_t score = -quiescence_search(remaining_ply, -beta, -alpha);
        pop();
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
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

static int32_t negamax(int16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line) {
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
    order_moves(moves, n);
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
        return quiescence_search(qsearch_lim, alpha, beta);
    }
    int32_t value = MIN_SCORE;
    size_t best_move_index = 0;
    std::vector<move_t> subsequent_lines[n];
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        push(candidate_move);
        subsequent_lines[i].push_back(candidate_move);
        int32_t sub_score = -negamax(reduction(candidate_move.score, remaining_ply),
                                     -beta, -alpha, &(subsequent_lines[i]));
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

/**
 * Implements Late Move Reduction for losing captures.
 * @param score Rated "goodness" or "interesting-ness" of a particular move.
 * @param current_ply Current ply remaining to search
 * @return The new depth to search at.
 */

int16_t reduction(int16_t score, int16_t current_ply) {
    const int16_t RF = 200;
    const uint16_t no_reduction = 2;

    if (current_ply <= no_reduction) {
        return current_ply - 1;
    }
    return std::max(0, current_ply - std::max(1, abs(std::min(0, (score + RF - 1) / RF))));
}

void order_moves(move_t moves[], int n) {
    for (int i = 0; i < n; ++i) {
        moves[i].compute_score();
    }
    std::sort(moves, moves + n);

}

int move_SEE(move_t move) {
    bitboard curr_board = board;
    int score = PAWN_MATERIAL * (move.flag == EN_PASSANT) + value(move.to);
    make_move(move);
    if (move.flag >= PR_KNIGHT && move.flag <= PR_QUEEN) {
        score += value(move.to);
    }
    score -= SEE(move.to);
    board = curr_board;
    return score;
}

/**
 * Static exchange evaluation:
 * @param move a move that captures an opponent's piece
 * @return returns whether or not the capture does not lose material
 */

int SEE(int square) {
    int see = 0;
    move_t lva_move = find_lva(square);
    if (lva_move.flag != PASS) {
        int cpv = value(square);
        make_move(lva_move);
        int prom_value = (lva_move.flag >= PC_KNIGHT) * value(lva_move.to);
        see = std::max(0, prom_value + cpv - SEE(square));
    }
    return see;
}

move_t find_lva(int square) {
    move_t recaptures[MAX_ATTACK_NUM];
    int num_recaptures = gen_legal_captures_sq(recaptures, board.turn, 1ULL << square);

    if (!num_recaptures) {
        /** There does not exist any moves that recapture on the given square */
        return NULL_MOVE;
    }
    int lva_index = 0;
    for (int i = 1; i < num_recaptures; ++i) {
        if (value(recaptures[i].from) < value(recaptures[lva_index].from)) {
            lva_index = i;
        }
    }
    return recaptures[lva_index];
}

/**
 * Returns the value of the piece moved in centipawns.
 * @param move self-explanatory
 * @return the value of the piece moved in centipawns
 */

int value(int square) {
    char piece = toupper(board.mailbox[square]);
    switch (piece) {
        case 'P':
            return PAWN_MATERIAL;
        case 'N':
            return KNIGHT_MATERIAL;
        case 'B':
            return BISHOP_MATERIAL;
        case 'R':
            return ROOK_MATERIAL;
        case 'Q':
            return QUEEN_MATERIAL;
        default:
            return 0;
    }
}

int value(move_t move) {
    switch (move.flag) {
        case EN_PASSANT:
            return PAWN_MATERIAL;
        case CAPTURE:
            return value(move.to);
        case PR_KNIGHT:
            return KNIGHT_MATERIAL;
        case PR_BISHOP:
            return BISHOP_MATERIAL;
        case PR_ROOK:
            return ROOK_MATERIAL;
        case PR_QUEEN:
            return QUEEN_MATERIAL;
        case PC_KNIGHT:
            return KNIGHT_MATERIAL + value(move.to);
        case PC_BISHOP:
            return BISHOP_MATERIAL + value(move.to);
        case PC_ROOK:
            return ROOK_MATERIAL + value(move.to);
        case PC_QUEEN:
            return QUEEN_MATERIAL + value(move.to);
        default:
            return 0;
    }
}

info_t search(int16_t depth) {
    top_line.clear();
    int32_t evaluation = negamax(depth, MIN_SCORE, -MIN_SCORE, &top_line);
    info_t reply = {.score = (1 - 2 * (board.turn == BLACK)) * evaluation, .best_move = top_line.front()};
    for (auto & i : top_line) {
        print_move(i);
        std::cout << ", ";
    }
    return reply;
}