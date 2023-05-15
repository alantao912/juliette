#include <chrono>
#include <cstring>
#include <pthread.h>
#include <algorithm>
#include <random>
#include <unordered_map>

#include "bitboard.h"
#include "evaluation.h"
#include "movegen.h"
#include "search.h"
#include "stack.h"
#include "tables.h"
#include "util.h"
#include "weights.h"

#define MIN_SCORE (INT32_MIN + 1000)
#define MATE_SCORE(depth) (MIN_SCORE + INT16_MAX - depth)
#define DRAW (int32_t) contempt_value;
#define MAX_DEPTH 128

/**
 * Quiescent search_fd max ply count.
 */

const int16_t qsearch_lim = 6;

/**
 * Contempt factor indicates respect for opponent.
 * Positive contempt indicates respect for stronger opponent.
 * Negative contempt indicates perceived weaker opponent
 */
const int32_t contempt_value = 0;

static std::unordered_map<uint64_t, TTEntry> transposition_table;
static pthread_mutex_t tt_lock, rand_lock;

volatile bool time_remaining = true, block_thread = true;

/** Following fields are thread local */
thread_local std::unordered_map<uint64_t, RTEntry> repetition_table;

static __thread std::vector<move_t> *killer_mvs = nullptr;
static __thread int16_t search_depth;
static __thread int32_t h_table[HTABLE_LEN];

__thread bitboard board;
__thread stack_t *stack = nullptr;
__thread int16_t ply = 0;

extern UCI::info_t result;

bool verify_repetition(uint64_t hash) {
    uint8_t num_seen = 0;
    stack_t *iterator = stack;
    while (iterator) {
        if (iterator->board.hash_code == hash) {
            /**
             * When checking if two positions are equal, it could be the case that two positions are equal, but have
             * different half move and full move number values. Thus, before and after using strncmp, we save and
             * restore the true values of the two variables.
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
 * Determines and returns whether the game's current state is a draw based off of the 50 move rule,
 * and 3-fold repetition.
 * @return Whether the game's current state is a draw.
 */

bool is_drawn() {
    const std::unordered_map<uint64_t, RTEntry>::const_iterator iterator = repetition_table.find(board.hash_code);
    if (iterator != repetition_table.end()) {
        const RTEntry &rt_entry = iterator->second;
        if (rt_entry.num_seen >= 3) {
            return verify_repetition(board.hash_code);
        }
    }
    return board.halfmove_clock >= 100;
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
 * Implements Late Move Reduction for moves with negative SEE.
 * @param score Rated "goodness" or "interesting-ness" of a particular move.
 * @param current_ply Current ply remaining to search_fd
 * @return The new depth to search at.
 */

int16_t reduction(int32_t score, int16_t current_ply) {
    const int16_t RF = 200, no_reduction = 2;
    /** TODO: Update reduction formula as new features are added*/

    if (current_ply <= no_reduction || score >= 0) {
        return current_ply - 1; // NOLINT
    }
    score += WIN_EX_SCORE; // Normalize the move's score value.
    return std::max((int16_t) 0, (int16_t) (current_ply - std::max(1, abs(std::min(0, (score + RF - 1) / RF)))));
}

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

int32_t piece_value(piece_t p) {
    switch (p) {
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

uint64_t find_lva(uint64_t attadef, bool turn, piece_t &piece) {
    int32_t min_piece_value = INT32_MAX;
    uint64_t lva_bb = 0ULL;
    while (attadef) {
        int i = pull_lsb(&attadef), value;

        if ((static_cast<int> (board.mailbox[i]) / 6) == turn &&
            (value = piece_value(board.mailbox[i])) < min_piece_value) {
            min_piece_value = value;

            lva_bb = BB_SQUARES[i];
            piece = board.mailbox[i];
        }
    }
    return lva_bb;
}

uint64_t consider_xray_attacks(int from, int to, const uint64_t occupied) {
    int shift_amt;
    uint64_t iterator = BB_SQUARES[from], boundary;
    if (from > to) {
        /** Left-Shift*/
        bool top_left = file_of(from) < file_of(to);
        bool top = file_of(from) == file_of(to);
        bool right = rank_of(from) == rank_of(to);
        bool top_right = (file_of(from) > file_of(to)) & (rank_of(from) > rank_of(to));
        shift_amt = 7 * top_left + 8 * top + right + 9 * top_right;
        boundary = ((top_right | right) * BB_FILE_A) + (top_left * BB_FILE_H);
        do {
            iterator = (iterator << shift_amt) & ~boundary;
            if (iterator & occupied)
                return iterator;
        } while (iterator);
    } else {
        /** Right-Shift */
        bool bottom_right = file_of(from) > file_of(to);
        bool bottom = file_of(from) == file_of(to);
        bool left = rank_of(from) == rank_of(to);
        bool bottom_left = ((file_of(from) < file_of(to)) & (rank_of(from) < rank_of(to)));
        shift_amt = left + 7 * bottom_right + 8 * bottom + 9 * bottom_left;
        boundary = (BB_FILE_A * bottom_right) + (BB_FILE_H * (left | bottom_left));
        do {
            iterator = (iterator >> shift_amt) & ~boundary;
            if (iterator & occupied)
                return iterator;
        } while (iterator);
    }
    return 0ULL;
}

/**
 * Static exchange score:
 * @param move a move that captures an opponent's piece
 * @return returns whether or not the capture does not lose material
 */

int32_t move_SEE(move_t move) {
    int gain[32], d = 0;
    uint64_t max_Xray = board.w_pawns | board.b_pawns | board.w_bishops | board.b_bishops | board.w_rooks |
                        board.b_rooks | board.w_queens | board.b_queens;
    uint64_t from_bb = BB_SQUARES[move.from];
    uint64_t occupied_bb = board.occupied;
    uint64_t attadef = attacks_to(move.to, occupied_bb);
    gain[d] = piece_value(move.to) + Weights::PAWN_MATERIAL * (move.flag == EN_PASSANT);

    piece_t attacking_piece = board.mailbox[move.from];
    do {
        ++d;
        gain[d] = piece_value(attacking_piece) - gain[d - 1];
        if (std::max(-gain[d - 1], gain[d]) < 0) break;
        attadef ^= from_bb;
        occupied_bb ^= from_bb;
        if (from_bb & max_Xray) {
            attadef |= consider_xray_attacks(get_lsb(from_bb), move.to, occupied_bb); // TODO: Consider x-ray attacks
        }
        from_bb = find_lva(attadef, (bool) ((board.turn + d) % 2), attacking_piece);
    } while (from_bb);
    while (--d) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }
    return gain[0];
}

/**
 * Returns the move_value of the piece moved in centi-pawns.
 * @param move self-explanatory
 * @return the move_value of the piece moved in centi-pawns
 */

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

inline size_t h_table_index(const move_t &mv) {
    return 64 * int(board.mailbox[mv.from]) + mv.to;
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

void order_moves(move_t mvs[], int32_t mv_scores[], int n) {
    auto cmp = [&mv_scores](int i, int j) { return mv_scores[i] > mv_scores[j]; };
    pthread_mutex_lock(&tt_lock);
    std::unordered_map<uint64_t, TTEntry>::const_iterator it = transposition_table.find(
            board.hash_code), end = transposition_table.end();
    pthread_mutex_unlock(&tt_lock);
    const std::vector<move_t> &kmvs = killer_mvs[ply];

    move_t hash_move = NULL_MOVE;
    if (it != end) {
        const TTEntry &tte = it->second;
        hash_move = tte.best_move;
    }

    uint_fast8_t mv_order[MAX_MOVE_NUM];
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
    move_t temp_buff[MAX_MOVE_NUM];
    for (size_t i = 0; i < n; ++i) {
        temp_buff[i] = mvs[mv_order[i]];
    }
    for (size_t i = 0; i < n; ++i) {
        mvs[i] = temp_buff[i];
    }
}

/**
 * @brief Extends the search_fd position until a "quiet" position is reached.
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
        return MATE_SCORE(depth + search_depth);
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

/**
 * @brief Returns an integer move_value, representing the score of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

#pragma ide diagnostic ignored "misc-no-recursion"

static int32_t pvs(int16_t depth, int32_t alpha, int32_t beta, move_t *mv_hst) {
    const int32_t original_alpha = alpha;

    pthread_mutex_lock(&tt_lock);
    std::unordered_map<uint64_t, TTEntry>::iterator t = transposition_table.find(board.hash_code);
    std::unordered_map<uint64_t, TTEntry>::const_iterator end = transposition_table.end();
    pthread_mutex_unlock(&tt_lock);

    if (t != end && t->second.depth >= depth) {
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
        /** Extend the search_fd until the position is quiet */
        return qsearch(qsearch_lim, alpha, beta);
    }
    /** Move generation logic */
    move_t mvs[MAX_MOVE_NUM];
    int n = gen_legal_moves(mvs, board.turn);

    /** Check game-lookahead terminating conditions */
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
    int32_t mv_scores[MAX_MOVE_NUM];
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
        /** If mvs[i] turns out to be better, re-search_fd with full window*/
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

    pthread_mutex_lock(&tt_lock);
    if (t != transposition_table.end()) {
        t->second = tt_entry;
    } else {
        transposition_table.insert(std::pair<uint64_t, TTEntry>(board.hash_code, tt_entry));
    }
    pthread_mutex_unlock(&tt_lock);
    killer_mvs[ply + 1].clear();
    return best_score;
}

UCI::info_t generate_reply(int32_t evaluation, move_t best_move) {
    UCI::info_t reply = {.score = (1 - 2 * (board.turn == BLACK)) * evaluation, .best_move = NULL_MOVE};
    if (!(best_move.from == A1 && best_move.to == A1 && best_move.flag == NONE)) {
        /** Not stalemate or checkmate */
        reply.best_move = best_move;
    } else if (abs(reply.score) == contempt_value) {
        /** Stalemate */
        reply.best_move = STALEMATE;
    } else {
        /** Checkmate */
        reply.best_move = CHECKMATE;
    }
    return reply;
}

void search_t(thread_args_t *args) {
    board = *(args->main_board);

    std::unordered_map<uint64_t, RTEntry>::const_iterator it = args->main_repetition_table->begin();
    const std::unordered_map<uint64_t, RTEntry>::const_iterator end = args->main_repetition_table->end();
    while (it != end) {
        repetition_table.insert(std::pair<uint64_t, RTEntry>(it->first, it->second));
        ++it;
    }
    const bool is_main_thread = args->is_main_thread;
    if (is_main_thread) {
        /** Main thread initializes shared locks */
        block_thread = true;
        pthread_mutex_init(&tt_lock, nullptr);
        pthread_mutex_init(&rand_lock, nullptr);
        block_thread = false;
    } else {
        /** Busy-wait for lock initialization to complete */
        while (block_thread);
    }

    move_t pv[MAX_DEPTH];
    pv[0] = NULL_MOVE;
    ply = 0;

    std::vector<move_t> kmv[MAX_DEPTH];
    killer_mvs = kmv;
    memset(h_table, 0, sizeof(int) * HTABLE_LEN);

    move_t root_mvs[MAX_MOVE_NUM];
    int n = gen_legal_moves(root_mvs, board.turn);

    pthread_mutex_lock(&rand_lock);
    int seed = std::random_device()();
    std::mt19937 rng(seed);
    pthread_mutex_unlock(&rand_lock);
    std::shuffle(root_mvs, &(root_mvs[n]), rng);

    move_t mv_pv[MAX_DEPTH];

    for (int16_t d = 0; time_remaining && d < MAX_DEPTH - 1; ++d) {
        search_depth = d;
        int32_t evaluation = MIN_SCORE;

        for (int i = 0; i < n; ++i) {
            mv_pv[0] = root_mvs[i];
            push(mv_pv[0]);
            int32_t mv_score = -pvs(d, MIN_SCORE, -evaluation, &mv_pv[1]);
            pop();

            if (mv_score > evaluation) {
                evaluation = mv_score;
                memcpy(pv, mv_pv, (d + 1) * sizeof(move_t));
            }
        }
        TTEntry tt_entry(evaluation, d + 1, EXACT, pv[0]);
        pthread_mutex_lock(&tt_lock);
        transposition_table[board.hash_code] = tt_entry;
        pthread_mutex_unlock(&tt_lock);

        if (args->is_main_thread && time_remaining) {
            result.best_move = pv[0];
            result.score = evaluation;
        }
    }

    /** Thread tear-down code */
    killer_mvs = nullptr;
    if (is_main_thread) {
        pthread_mutex_destroy(&tt_lock);
        pthread_mutex_destroy(&rand_lock);
    }
    pthread_exit(nullptr);
}

UCI::info_t search_fd(int16_t depth) {
    move_t pv[depth];
    pv[0] = NULL_MOVE;
    ply = 0;

    std::vector<move_t> kmv[depth];
    killer_mvs = kmv;
    memset(h_table, 0, sizeof(int) * HTABLE_LEN);

    int32_t evaluation;
    for (int16_t i = 1; i <= depth; ++i) {
        search_depth = i;
        evaluation = pvs(i, MIN_SCORE, -MIN_SCORE, pv);
    }
    killer_mvs = nullptr;
    return generate_reply(evaluation, pv[0]);
}

UCI::info_t search_ft(std::chrono::duration<int64_t, std::milli> time) {
    move_t pv[MAX_DEPTH];
    pv[0] = NULL_MOVE;
    ply = 0;

    int kmv_len = 8;
    killer_mvs = new std::vector<move_t>[kmv_len];
    memset(h_table, 0, sizeof(int) * HTABLE_LEN);

    int32_t evaluation;
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    for (int16_t i = 1; time.count() > 0; ++i) {
        search_depth = i;
        evaluation = pvs(i, MIN_SCORE, -MIN_SCORE, pv);

        /** Update time */
        std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        time -= std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
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