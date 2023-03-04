#include <chrono>
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

#define MIN_SCORE (INT32_MIN + 1000)
#define MATE_SCORE(depth) (MIN_SCORE + INT16_MAX - depth)
#define DRAW (int32_t) contempt;
#define MAX_DEPTH 128

/**
 * Quiescent search max ply count.
 */

const int16_t qsearch_lim = 6;

/**
 * Contempt factor indicates respect for opponent.
 * Positive contempt indicates respect for stronger opponent.
 * Negative contempt indicates perceived weaker opponent
 */
const int32_t contempt = 0;

std::unordered_map<uint64_t, TTEntry> transposition_table;

/** Following fields are thread local */
// TODO: Wrap thread local fields into struct.
std::unordered_map<uint64_t, RTEntry> repetition_table;
std::vector<move_t> *killer_mvs = nullptr;
bitboard board;
stack_t *stack = nullptr;
int16_t ply = 0;
int16_t init_depth;
int32_t h_table[HTABLE_LEN];

bool is_drawn() {
    auto iterator = repetition_table.find(board.hash_code);
    if (iterator != repetition_table.end()) {
        const RTEntry &rt_entry = iterator->second;
        if (rt_entry.num_seen >= 3) {
            return verify_repetition(board.hash_code);
        }
    }
    return board.halfmove_clock >= 100;
}

bool verify_repetition(uint64_t hash) {
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

inline bool use_fprune(move_t cm, int16_t depth) {
    return false;
    // return depth == 1 && cm.score < CHECK_SCORE && stack->prev_mv.score < CHECK_SCORE;
}

static inline bool contains_promotions() {
    uint64_t prom_squares;
    if (board.turn) {
        /** Checks if white has any pawn promotions */
        prom_squares = ((board.w_pawns & BB_RANK_7) << 8) & ~board.occupied;
        prom_squares |= ((((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H)) & BB_RANK_8) &
                        board.b_occupied;
    } else {
        /** Checks if black has any pawn promotions */
        prom_squares = ((board.b_pawns & BB_RANK_2) >> 8) & ~board.occupied;
        prom_squares |= ((((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A)) & BB_RANK_1) &
                        board.w_occupied;
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
        return DRAW;
    }

    int32_t stand_pat = MIN_SCORE;

    move_t moves[MAX_MOVE_NUM];
    int n, n_checks;
    if (!is_check(board.turn)) {
        /** Generate non-quiet moves, such as checks, promotions, and captures. */
        if (depth < 0 || !(n = gen_nonquiescent_moves(moves, board.turn, &n_checks))) {
            /** Position is quiet, return score. */
            return evaluate();
        }
    } else if ((n = gen_legal_moves(moves, board.turn))) {
        /** Side to move is in check, evasions exist. */
        /** Effectively, this line disables futility pruning. No futility pruning in check. */
        n_checks = n;
        goto CHECK_EVASIONS;
    } else {
        /** Side to move is in check, evasions do not exist. Checkmate :( */
        return MATE_SCORE(depth + init_depth);
    }

    stand_pat = evaluate();
    if (stand_pat >= beta) {
        return beta;
    }

    {
        int big_delta = Weights::QUEEN_MATERIAL;
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
    for (size_t i = 0; i < n; ++i) {
        move_t candidate_move = moves[i];
        if (i >= n_checks && move_value(candidate_move) + stand_pat < alpha - DELTA_MARGIN) {
            /** Skip evaluating this move */
            continue;
        }
        push(candidate_move);
        int32_t score = -qsearch(depth - 1, -beta, -alpha);
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

/**
 * @brief Returns an integer move_value, representing the score of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

static int32_t pvs(int16_t depth, int32_t alpha, int32_t beta, move_t *mv_hst) {
    const int32_t original_alpha = alpha;
    std::unordered_map<uint64_t, TTEntry>::iterator t = transposition_table.find(board.hash_code);
    if (t != transposition_table.end() && t->second.depth >= depth) {
        const TTEntry &tt_entry = t->second;
        switch (tt_entry.flag) {
            case EXACT:
                *mv_hst = tt_entry.best_move;
                return tt_entry.score;
            case LOWER:
                alpha = std::max(alpha, tt_entry.score);
                break;
            case UPPER:
                beta = std::min(beta, tt_entry.score);
                break;
        }
    }
    if (depth == 0) {
        /** Extend the search until the position is quiet */
        return qsearch(qsearch_lim, alpha, beta);
    }
    /** Move generation logic */
    move_t mvs[MAX_MOVE_NUM];
    int n = gen_legal_moves(mvs, board.turn);

    /** Check game-lookahead, terminating conditions */
    if (n == 0) {
        if (is_check(board.turn)) {
            /** King is in check, and there are no legal mvs. Checkmate */
            return MATE_SCORE(depth);
        }
        /** No legal mvs, yet king is not in check. This is a stalemate, and the game is drawn. */
        return DRAW;
    }
    if (is_drawn()) {
        return DRAW;
    }

    /** Move ordering logic */
    int32_t mv_scores[n];
    // Permutation array for move ordering. Ex: mv_order[5] = 3, 5th move in the order is at index 3.
    order_moves(mvs, mv_scores, n);
    /** Begin PVS check first move */
    size_t pv_index = 0;
    move_t variations[depth];

    push(mvs[0]);
    variations[0] = mvs[0];
    int32_t best_score = -pvs(depth - 1, -beta, -alpha, &variations[1]);
    pop();

    if (best_score > alpha) {
        alpha = best_score;
        memcpy(mv_hst, variations, depth * sizeof(move_t));
    }

    if (alpha >= beta) {
        store_cutoff_mv(mvs[0], mv_scores[0]);
        goto END;
    }
    /** End PVS check first move */

    /** PVS check subsequent moves */
    for (size_t i = 1; i < n; ++i) {
        const move_t &mv = mvs[i];
        const int &mv_score = mv_scores[i];
        /** Futility pruning */
        if (use_fprune(mv, depth) && best_score + move_value(mv) < alpha - DELTA_MARGIN) {
            continue;
        }
        push(mv);
        variations[0] = mv;
        /** Zero-Window Search. Assume good move ordering, and all subsequent mvs are worse. */
        int32_t score = -pvs(reduction(mv_score, depth), -alpha - 1, -alpha, &variations[1]);
        /** If mvs[i] turns out to be better, re-search with full window*/
        if (alpha < score && score < beta) {
            score = -pvs(reduction(mv_score, depth), -beta, -score, &variations[1]);
        }
        pop();
        if (score > best_score) {
            best_score = score;
            pv_index = i;
        }
        /** Found new PV move */
        if (best_score > alpha) {
            alpha = score;
            memcpy(mv_hst, variations, depth * sizeof(move_t));
        }

        /** Beta-Cutoff */
        if (alpha >= beta) {
            store_cutoff_mv(mv, mv_score);
            break;
        }
    }
    END:
    /** Updates the transposition table with the appropriate values */
    TTEntry tt_entry(best_score, depth, EXACT, mvs[pv_index]);
    if (best_score <= original_alpha) {
        tt_entry.flag = UPPER;
    } else if (best_score >= beta) {
        tt_entry.flag = LOWER;
    }
    if (t != transposition_table.end()) {
        t->second = tt_entry;
    } else {
        transposition_table.insert(std::pair<uint64_t, TTEntry>(board.hash_code, tt_entry));
    }
    killer_mvs[ply + 1].clear();
    return best_score;
}

#pragma clang diagnostic pop

/**
 * Implements Late Move Reduction for moves with negative SEE.
 * @param score Rated "goodness" or "interesting-ness" of a particular move.
 * @param current_ply Current ply remaining to search
 * @return The new depth to search at.
 */

int16_t reduction(int32_t score, int16_t current_ply) {
    const int16_t RF = 200, no_reduction = 2;
    /** TODO: Update reduction formula as new features are added*/

    if (current_ply <= no_reduction) {
        return current_ply - 1; // NOLINT
    }
    score += WIN_EX_SCORE; // Normalize the move's score value.
    return std::max((int16_t) 0, (int16_t) (current_ply - std::max(1, abs(std::min(0, (score + RF - 1) / RF)))));
}

void order_moves(move_t mvs[], int32_t mv_scores[], int n) {
    auto cmp = [&mv_scores](int i, int j) { return mv_scores[i] > mv_scores[j]; };
    std::unordered_map<uint64_t, TTEntry>::iterator it = transposition_table.find(board.hash_code);
    const std::vector<move_t> &kmvs = killer_mvs[ply];

    move_t hash_move = NULL_MOVE;
    if (it != transposition_table.end()) {
        const TTEntry &tte = it->second;
        hash_move = tte.best_move;
    }

    uint_fast8_t mv_order[n];
    for (int i = 0; i < n; ++i) {
        mv_order[i] = i;
        const move_t &mv = mvs[i];
        int &score = mv_scores[i];
        if (mv == hash_move) {
            score = HM_SCORE;
            continue;
        } else if (std::find(kmvs.begin(), kmvs.end(), mv) != kmvs.end()) {
            score = KM_SCORE;
            continue;
        }

        score = 0;

        if (is_move_check(mv)) {
            score += CHECK_SCORE;
        }

        switch (mv.flag) {
            case CASTLING:
                break;
            case CAPTURE: {
                int diff = piece_value(mv.to) - piece_value(mv.from);
                if (diff > 0) {
                    score += WIN_EX_SCORE + diff;
                    break;
                }
            }
            default: {
                /** SEE determines whether or not the move wins or loses material */
                int32_t SEE = move_SEE(mv);
                score += (SEE > 0) * WIN_EX_SCORE + (SEE < 0) * NEG_EX_SCORE + SEE;
                score += (SEE >= 0) * h_table[h_table_index(mv)];
            }
        }
    }
    std::sort(mv_order, mv_order + n, cmp);
    move_t temp_buff[n];
    for (size_t i = 0; i < n; ++i) {
        temp_buff[i] = mvs[mv_order[i]];
    }
    for (size_t i = 0; i < n; ++i) {
        mvs[i] = temp_buff[i];
    }
}

void store_cutoff_mv(move_t mv, int32_t mv_score) {
    if (mv.flag == NONE) {
        /** If move is not a check, nor already a killer move since KM_SCORE < CHECK_SCORE.*/
        if (mv_score < KM_SCORE) {
            killer_mvs[ply].push_back(mv);
        }
        h_table[h_table_index(mv)] += ply * ply;
    }
}

size_t h_table_index(const move_t &mv) {
    return 64 * int(board.mailbox[mv.from]) + mv.to;
}

int32_t move_SEE(move_t move) {
    int32_t score = Weights::PAWN_MATERIAL * (move.flag == EN_PASSANT) + piece_value(move.to);
    bitboard temp = board;
    make_move(move);
    if (move.flag >= PR_KNIGHT && move.flag <= PR_QUEEN) {
        score += piece_value(move.to);
    }
    score -= SEE(move.to);
    board = temp;
    return score;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

/**
 * Static exchange score:
 * @param move a move that captures an opponent's piece
 * @return returns whether or not the capture does not lose material
 */

int32_t SEE(int square) {
    int32_t see = 0;
    move_t lva_move = find_lva(square);
    if (lva_move.flag != PASS) {
        int32_t cpv = piece_value(square);
        bitboard temp = board;
        make_move(lva_move);
        int32_t prom_value = (lva_move.flag >= PC_KNIGHT) * piece_value(lva_move.to);
        see = std::max(0, prom_value + cpv - SEE(square));
        board = temp;
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

int32_t piece_value(int square) {
    piece_t piece = static_cast<piece_t> (board.mailbox[square]);
    switch (piece) {
        case BLACK_PAWN:
            return Weights::PAWN_MATERIAL;
        case BLACK_KNIGHT:
            return Weights::KNIGHT_MATERIAL;
        case BLACK_BISHOP:
            return Weights::BISHOP_MATERIAL;
        case BLACK_ROOK:
            return Weights::ROOK_MATERIAL;
        case BLACK_QUEEN:
            return Weights::QUEEN_MATERIAL;
        case WHITE_PAWN:
            return Weights::PAWN_MATERIAL;
        case WHITE_KNIGHT:
            return Weights::KNIGHT_MATERIAL;
        case WHITE_BISHOP:
            return Weights::BISHOP_MATERIAL;
        case WHITE_ROOK:
            return Weights::ROOK_MATERIAL;
        case WHITE_QUEEN:
            return Weights::QUEEN_MATERIAL;
        default:
            return 0;
    }
}

int32_t move_value(move_t move) {
    switch (move.flag) {
        case EN_PASSANT:
            return Weights::PAWN_MATERIAL;
        case CAPTURE:
            return piece_value(move.to);
        case PR_KNIGHT:
            return Weights::KNIGHT_MATERIAL;
        case PR_BISHOP:
            return Weights::BISHOP_MATERIAL;
        case PR_ROOK:
            return Weights::ROOK_MATERIAL;
        case PR_QUEEN:
            return Weights::QUEEN_MATERIAL;
        case PC_KNIGHT:
            return Weights::KNIGHT_MATERIAL + piece_value(move.to);
        case PC_BISHOP:
            return Weights::BISHOP_MATERIAL + piece_value(move.to);
        case PC_ROOK:
            return Weights::ROOK_MATERIAL + piece_value(move.to);
        case PC_QUEEN:
            return Weights::QUEEN_MATERIAL + piece_value(move.to);
        default:
            return 0;
    }
}

info_t generate_reply(int32_t evaluation, move_t best_move) {
    info_t reply = {.score = (1 - 2 * (board.turn == BLACK)) * evaluation, .best_move = NULL_MOVE};
    if (!(best_move.from == A1 && best_move.to == A1 && best_move.flag == NONE)) {
        /** Not stalemate or checkmate */
        reply.best_move = best_move;
    } else if (abs(reply.score) == contempt) {
        /** Stalemate */
        reply.best_move = STALEMATE;
    } else {
        /** Checkmate */
        reply.best_move = CHECKMATE;
    }
    return reply;
}

info_t search(int16_t depth) {
    move_t pv[depth];
    pv[0] = NULL_MOVE;
    ply = 0;
    init_depth = depth;
    std::vector<move_t> kmv[depth];
    killer_mvs = kmv;
    memset(h_table, 0, sizeof(int) * HTABLE_LEN);

    int32_t evaluation;
    for (int16_t i = 1; i <= depth; ++i) {
        evaluation = pvs(i, MIN_SCORE, -MIN_SCORE, pv);
    }
    killer_mvs = nullptr;
    return generate_reply(evaluation, pv[0]);
}

info_t search(std::chrono::duration<int64_t, std::milli> time_ms) {
    move_t pv[MAX_DEPTH];
    pv[0] = NULL_MOVE;
    ply = 0;

    int kmv_len = 8;
    killer_mvs = new std::vector<move_t>[kmv_len];
    memset(h_table, 0, sizeof(int) * HTABLE_LEN);

    int32_t evaluation;
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    for (int16_t i = 1; time_ms.count() > 0; ++i) {
        init_depth = i;
        evaluation = pvs(i, MIN_SCORE, -MIN_SCORE, pv);

        /** Update time */
        std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        time_ms -= std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        start = now;

        if (i > kmv_len) {
            /** Resize killer move array */
            std::vector<move_t> *buff = new std::vector<move_t>[2 * kmv_len];
            for (size_t j = 0; j < kmv_len; ++j) {
                buff[j] = killer_mvs[j];
            }
            delete[] killer_mvs;
            killer_mvs = buff;
        }
    }
    delete[] killer_mvs;
    return generate_reply(evaluation, pv[0]);
}