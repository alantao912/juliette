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

const int16_t qsearch_lim = 4;

/**
 * Contempt factor indicates respect for opponent.
 * Positive contempt indicates respect for stronger opponent.
 * Negative contempt indicates perceived weaker opponent
 */
const int32_t contempt = 0;

#define MIN_SCORE (INT32_MIN + 1000)
#define CHECKMATE(depth) ((MIN_SCORE + 1) + (UINT16_MAX - depth))
#define DRAW (int32_t) contempt;


std::unordered_map<uint64_t, TTEntry> transposition_table;
std::unordered_map<uint64_t, RTEntry> repetition_table;

extern bitboard board;
extern stack_t *stack;

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

/**
 * Determines whether to enable futility pruning.
 * @param cm Candidate move
 * @param depth Depth remaining until horizon
 * @return Returns whether depth == 1, candidate move is not a check, and previous move was not a check (we are moving out of check)
 */

static inline bool use_fprune(move_t cm, int16_t depth) {
    return false;
    return depth == 1 && cm.score < CHECK_SCORE && stack->prev_mv.score < CHECK_SCORE;
}

static inline bool contains_promotions() {
    uint64_t prom_squares;
    if (board.turn) {
        /** Checks if white has any pawn promotions */
        prom_squares = ((board.w_pawns & BB_RANK_7) << 8) & ~board.occupied;
        prom_squares |= ((((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H)) & BB_RANK_8) & board.b_occupied;
    } else {
        /** Checks if black has any pawn promotions */
        prom_squares = ((board.b_pawns & BB_RANK_2) >> 8) & ~board.occupied;
        prom_squares |= ((((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A)) & BB_RANK_1) & board.w_occupied;
    }
    return pop_count(prom_squares) > 0;
}

/**
 * @brief Extends the search position until a "quiet" position is reached.
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 * @return
 */

int32_t qsearch(int16_t depth, int32_t alpha, int32_t beta) { // NOLINT
    if (is_drawn()) {
        return DRAW; // NOLINT
    }

    int32_t stand_pat = MIN_SCORE;

    move_t moves[MAX_MOVE_NUM];
    int n, n_checks;
    if (!is_check(board.turn)) {
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        if (!depth || !(n = gen_nonquiescent_moves(moves, board.turn, &n_checks))) {
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
        return CHECKMATE(depth);
    }

    --depth;
    stand_pat = evaluate();
    if (stand_pat >= beta) {
        return beta;
    }

    {
        int big_delta = Weights::queen_material;
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
        if (i >= n_checks && move_value(candidate_move) + stand_pat < alpha - DELTA_MARGIN) {
            /** Skip evaluating this move */
            continue;
        }
        push(candidate_move);
        int32_t score = -qsearch(depth, -beta, -alpha);
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
 * @brief Returns an integer move_value, representing the evaluation of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

static int32_t negamax(int16_t depth, int32_t alpha, int32_t beta, std::vector<move_t> &mv_hst) { // NOLINT
    const int32_t original_alpha = alpha;
    std::unordered_map<uint64_t, TTEntry>::iterator t = transposition_table.find(board.hash_code); // NOLINT
    if (t != transposition_table.end() && t->second.depth >= depth) {
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
            return CHECKMATE(depth);
        }
        /** No legal moves, yet king is not in check. This is a stalemate, and the game is drawn. */
        return DRAW; // NOLINT
    }
    if (is_drawn()) {
        return DRAW; // NOLINT
    }
    if (!depth) {
        /** Extend the search until the position is quiet */
        return qsearch(qsearch_lim, alpha, beta);
    }
    int32_t score = MIN_SCORE;
    size_t pv_index = 0;
    std::vector<move_t> variations[n];
    for (size_t i = 0; i < n; ++i) {
        move_t mv = moves[i];

        if (use_fprune(mv, depth) && score + move_value(mv) < alpha - DELTA_MARGIN) {
            continue;
        }

        push(mv);
        variations[i].push_back(mv);
        int32_t sub_score = -negamax(reduction(mv.score, depth),-beta, -alpha, variations[i]);
        pop();
        if (sub_score > score) {
            score = sub_score;
            pv_index = i;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            goto PRUNE;
        }
    }
    /** Propagates up the principal variation */
    for (move_t m : variations[pv_index]) {
        mv_hst.push_back(m);
    }
    PRUNE:
    /** Updates the transposition table with the appropriate values */
    TTEntry tt_entry(0, 0, EXACT);
    if (score <= original_alpha) {
        tt_entry.flag = UPPER;
    } else if (score >= beta) {
        tt_entry.flag = LOWER;
    }
    tt_entry.depth = depth;
    if (t != transposition_table.end()) {
        t->second = tt_entry;
    } else {
        transposition_table.insert(std::pair<uint64_t, TTEntry>(board.hash_code, tt_entry));
    }
    return score;
}

/**
 * Implements Late Move Reduction for moves with negative SEE.
 * @param score Rated "goodness" or "interesting-ness" of a particular move.
 * @param current_ply Current ply remaining to search
 * @return The new depth to search at.
 */

int16_t reduction(int16_t score, int16_t current_ply) {
    const int16_t RF = 200, no_reduction = 2;
    /** TODO: Update reduction formula as new features are added*/

    if (current_ply <= no_reduction) {
        return current_ply - 1; // NOLINT
    }
    return std::max((int16_t) 0, (int16_t) (current_ply - std::max(1, abs(std::min(0, (score + RF - 1) / RF)))));
}

void order_moves(move_t moves[], int n) {
    for (int i = 0; i < n; ++i) {
        moves[i].compute_score();
    }
    std::sort(moves, moves + n);

}

int move_SEE(move_t move) {
    bitboard curr_board = board;
    int score = Weights::pawn_material * (move.flag == EN_PASSANT) + piece_value(move.to);
    make_move(move);
    if (move.flag >= PR_KNIGHT && move.flag <= PR_QUEEN) {
        score += piece_value(move.to);
    }
    score -= SEE(move.to);
    board = curr_board;
    return score;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
/**
 * Static exchange evaluation:
 * @param move a move that captures an opponent's piece
 * @return returns whether or not the capture does not lose material
 */

int SEE(int square) {
    int see = 0;
    move_t lva_move = find_lva(square);
    if (lva_move.flag != PASS) {
        int cpv = piece_value(square);
        make_move(lva_move);
        int prom_value = (lva_move.flag >= PC_KNIGHT) * piece_value(lva_move.to);
        see = std::max(0, prom_value + cpv - SEE(square));
    }
    return see;
}
#pragma clang diagnostic pop

move_t find_lva(int square) {
    move_t recaptures[MAX_ATTACK_NUM];
    int num_recaptures = gen_legal_captures_sq(recaptures, board.turn, 1ULL << square);

    if (!num_recaptures) {
        /** There does not exist any moves that recapture on the given square */
        return NULL_MOVE;
    }
    int lva_index = 0;
    for (int i = 1; i < num_recaptures; ++i) {
        if (piece_value(recaptures[i].from) < piece_value(recaptures[lva_index].from)) {
            lva_index = i;
        }
    }
    return recaptures[lva_index];
}

/**
 * Returns the move_value of the piece moved in centipawns.
 * @param move self-explanatory
 * @return the move_value of the piece moved in centipawns
 */

inline int piece_value(int square) {
    char piece = (char) toupper(board.mailbox[square]);
    switch (piece) {
        case 'P':
            return Weights::pawn_material;
        case 'N':
            return Weights::knight_material;
        case 'B':
            return Weights::bishop_material;
        case 'R':
            return Weights::rook_material;
        case 'Q':
            return Weights::queen_material;
        default:
            return 0;
    }
}

int move_value(move_t move) {
    switch (move.flag) {
        case EN_PASSANT:
            return Weights::pawn_material;
        case CAPTURE:
            return piece_value(move.to);
        case PR_KNIGHT:
            return Weights::knight_material;
        case PR_BISHOP:
            return Weights::bishop_material;
        case PR_ROOK:
            return Weights::rook_material;
        case PR_QUEEN:
            return Weights::queen_material;
        case PC_KNIGHT:
            return Weights::knight_material + piece_value(move.to);
        case PC_BISHOP:
            return Weights::bishop_material + piece_value(move.to);
        case PC_ROOK:
            return Weights::rook_material + piece_value(move.to);
        case PC_QUEEN:
            return Weights::queen_material + piece_value(move.to);
        default:
            return 0;
    }
}

info_t search(int16_t depth) {
    std::vector<move_t> top_line;
    top_line.clear();
    int32_t evaluation = negamax(depth, MIN_SCORE, -MIN_SCORE, top_line);
    info_t reply = {.score = (1 - 2 * (board.turn == BLACK)) * evaluation, .best_move = NULL_MOVE};
    if (!top_line.empty()) {
        /** Not stalemate or checkmate */
        reply.best_move = top_line.front();
    } else if (abs(reply.score) == contempt) {
        /** Stalemate */
        reply.best_move = STALEMATE;
    } else {
        /** Checkmate */
        reply.best_move = CHECKMATE;
    }
    return reply;
}