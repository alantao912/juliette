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
#include "timeman.h"
#include "uci.h"
#include "util.h"
#include "weights.h"

#define MIN_SCORE (INT32_MIN + 1000)
#define MATE_SCORE(depth) (MIN_SCORE + INT16_MAX + depth)


pthread_mutex_t SearchContext::init_lock;
TTable SearchContext::transpositionTable;
const int32_t SearchContext::contempt_value = 0;

const UCI *SearchContext::uciInstance = nullptr;

volatile size_t SearchContext::nActiveThreads = 0;

bool SearchContext::timeRemaining = false;
bool SearchContext::blockHelpers = false;

/**
 * Verifies three-fold repetition claimed by the repetition table.
 * @param hash Hash-code of the position to check
 * @return whether the three-fold repetition of the position provided by the hash-code is valid.
 */

bool SearchContext::verifyRepetition(uint64_t hash) {
    int numSeen = 0;
    stack_t *iterator = this->stack;
    while (iterator) {
        if (iterator->board.getHashCode() == hash) {
            numSeen += int(iterator->board == this->board);
            if (numSeen >= 3) return true;
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

bool SearchContext::isDrawn() {
    const std::unordered_map<uint64_t, RTEntry>::const_iterator iterator = this->repetitionTable.find(this->board.getHashCode());
    if (iterator != repetitionTable.end()) {
        const RTEntry &rt_entry = iterator->second;
        if (rt_entry.num_seen >= 3) {
            return this->verifyRepetition(this->board.getHashCode());
        }
    }
    return this->board.getHalfmoveClock() >= 100;
}

/**
 * Determines whether to enable futility pruning.
 * @param cm Candidate move
 * @param depth Depth remaining until horizon
 * @return Returns whether depth == 1, candidate move is not a check, and previous move was not a check (we are moving out of check)
 */

bool SearchContext::useFutilityPruning(move_t cm, int16_t depth) {
    return depth == 1 && !cm.isType(move_t::type_t::CHECK_MOVE) && !this->stack->previousMove.isType(move_t::type_t::CHECK_MOVE);
}

/**
 * Computes the index of a given move in the history table using butterfly boards. That is,
 * h_table[from][to]. Where 'from' and 'to' are converted to an index on [0, 63].
 * @param mv Move to compute index for.
 * @return Index of given move in the history table.
 */

size_t SearchContext::hTableIndex(const move_t &mv) {
    return 64 * size_t(this->board.lookupMailbox(mv.from)) + size_t(mv.to);
}

const info_t &SearchContext::getResult() {
    return SearchContext::result;
}

/**
 * Implements Late Move Reduction.
 * @param move Move to determine compute reduction ply count.
 * @param current_ply Current ply remaining to search.
 * @param n Move index number after move ordering.
 * @return The amount by which to reduce current ply.
 */

int16_t SearchContext::computeReduction(move_t mv, int16_t current_ply, int i) {
    const int16_t no_reduction = 3;
    const int NO_LMR = 4;
    /** If there are two plies or fewer to horizon, giving check, or in check, do not reduce. */
    if (current_ply < no_reduction || i < NO_LMR ||
        mv.isType(move_t::type_t::CHECK_MOVE) || this->stack->previousMove.isType(move_t::type_t::CHECK_MOVE)) {
        return 0;
    }

    /** If move is a capture or promotion (tactical possibilities), do not reduce. */
    if (mv.flag >= MoveFlags::EN_PASSANT && !mv.isType(move_t::type_t::LOSING_EXCHANGE)) {
        return 0;
    }

    /** int32_t material_loss_rf: Material loss in centi-pawns resulting in one additional ply reduction */
    const int32_t material_loss_rf = 250;
    int16_t reduction = 1 + int16_t(sqrt(current_ply - 1) + sqrt(i - 1));
    if (mv.isType(move_t::LOSING_EXCHANGE)) {
        const int32_t loss = -mv.normalizeScore();
        const int16_t inc = (loss - 1) / material_loss_rf + 1;
        reduction += inc;
    }
    return reduction;
}

int32_t SearchContext::pieceValue(piece_t p) {
    size_t index = static_cast<size_t> (p) % 6;
    return Weights::MATERIAL[index];
}

/**
 * Returns the move value of the piece moved in centi-pawns.
 * @param move the move to consider value of
 * @return the move value of the piece moved in centi-pawns
 */

int32_t SearchContext::moveValue(move_t move) {
    switch (move.flag) {
        case EN_PASSANT:
            return Weights::MATERIAL[piece_t::BLACK_PAWN];
        case CAPTURE:
            return this->board.pieceValue(move.to);
        case PR_KNIGHT:
            return Weights::MATERIAL[piece_t::BLACK_KNIGHT];
        case PR_BISHOP:
            return Weights::MATERIAL[piece_t::BLACK_BISHOP];
        case PR_ROOK:
            return Weights::MATERIAL[piece_t::BLACK_ROOK];
        case PR_QUEEN:
            return Weights::MATERIAL[piece_t::BLACK_QUEEN];
        case PC_KNIGHT:
            return Weights::MATERIAL[piece_t::BLACK_KNIGHT] + this->board.pieceValue(move.to);
        case PC_BISHOP:
            return Weights::MATERIAL[piece_t::BLACK_BISHOP] + this->board.pieceValue(move.to);
        case PC_ROOK:
            return Weights::MATERIAL[piece_t::BLACK_ROOK] + this->board.pieceValue(move.to);
        case PC_QUEEN:
            return Weights::MATERIAL[piece_t::BLACK_QUEEN] + this->board.pieceValue(move.to);
        default:
            return 0;
    }
}

void SearchContext::storeCutoffMove(move_t mv, int16_t depth) {
    if (mv.isType(move_t::type_t::QUIET)) {
        if (!mv.isType(move_t::type_t::KILLER_MOVE)) {
            this->killerMoves[ply].push_back(mv);
        }
        this->historyTable[hTableIndex(mv)] += depth * depth;
    }
}

void SearchContext::orderMoves(move_t mvs[], int n) {
    const TTEntry *it = SearchContext::transpositionTable.find(board.getHashCode());
    const std::vector<move_t> &kmvs = killerMoves[ply];

    move_t hash_move = NULL_MOVE;
    if (it != nullptr) {
        hash_move = it->best_move;
    }

    for (int i = 0; i < n; ++i) {
        move_t &mv = mvs[i];
        if (mv == hash_move) {
            mv.setScore(move_t::type_t::HASH_MOVE, 0);
            continue;
        } else if (std::find(kmvs.begin(), kmvs.end(), mv) != kmvs.end()) {
            mv.setScore(move_t::type_t::KILLER_MOVE, 0);
            continue;
        }

        if (board.isMoveCheck(mv)) {
            mv.setScore(move_t::type_t::CHECK_MOVE, 0);
        }

        switch (mv.flag) {
            case CASTLING: {
                mv.setScore(move_t::type_t::QUIET, this->historyTable[this->hTableIndex(mv)]);
                break;
            }
            case CAPTURE: {
                int diff = this->board.pieceValue(mv.to) - this->board.pieceValue(mv.from);
                if (diff > 0) {
                    mv.setScore(move_t::type_t::WINNING_EXCHANGE, diff);
                    break;
                }
            }
            default: {
                /** fastSEE determines whether or not the move wins or loses material */
                int32_t see_score = this->board.fastSEE(mv);
                if (see_score < 0) {
                    /** Quiet move that loses material, or losing exchange. */
                    mv.setScore(move_t::type_t::LOSING_EXCHANGE, see_score);
                } else if (mv.flag == NONE) {
                    /** Quiet move. Cannot be a move that wins material since that would require capture. */
                    mv.setScore(move_t::type_t::QUIET, this->historyTable[this->hTableIndex(mv)]);
                } else {
                    /** Non-quiet move that wins, or maintains equal material */
                    mv.setScore(move_t::type_t::WINNING_EXCHANGE, see_score);
                }
            }
        }
    }
    std::sort(mvs, mvs + n);
}

void SearchContext::pushMove(const move_t &mv) {
    stack_t *node = new stack_t(this->board, mv);
    node->next = this->stack;
    this->stack = node;
    this->board.makeMove(mv);
    std::unordered_map<uint64_t, RTEntry>::iterator rtPair = repetitionTable.find(this->board.getHashCode());
    if (rtPair != repetitionTable.end()) {
        RTEntry &rtEntry = rtPair->second;
        ++rtEntry.num_seen;
    } else {
        repetitionTable.insert(std::pair<uint64_t, RTEntry>(board.getHashCode(), RTEntry(1)));
    }
    ++(this->ply);
}

void SearchContext::popMove() {
    RTEntry &rtPair = repetitionTable.find(this->board.getHashCode())->second;
    --rtPair.num_seen;
    stack_t *head = this->stack;
    this->board = head->board;
    this->stack = head->next;
    --(this->ply);
    delete head;
}

/**
 * @brief Extends the search_fd position until a "quiet" position is reached.
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 * @return The static evaluation 
 */

int32_t SearchContext::qsearch(int32_t alpha, int32_t beta) { // NOLINT
    int32_t stand_pat;

    int n;
    move_t moves[MAX_MOVE_NUM];

    bool in_check = false;
    if (!board.isInCheck(board.getTurn())) {
        /** Generate non-quiet moves, such as promotions, and captures. */
        n = this->board.genNonquiescentMoves(moves, this->board.getTurn()); // TODO Refactor movegen
        if (n == 0) {
            /** Position is quiet, return score. */
            return position.evaluate();
        }
    } else if ((n = this->board.genLegalMoves(moves, this->board.getTurn()))) { // TODO Refactor movegen
        /** Side to move is in check, evasions exist. */
        in_check = true;
        goto CHECK_EVASIONS;
    } else {
        /** Side to move is in check, evasions do not exist. Checkmate :( */
        return MATE_SCORE(ply);
    }

    /** Block: Only runs if not in check, and non-quiet moves exist. */

    stand_pat = position.evaluate();
    if (stand_pat >= beta) {
        return beta;
    }

    {
        int big_delta = Weights::MATERIAL[piece_t::BLACK_QUEEN];
        if (this->board.containsPromotions()) {
            big_delta += 775;
        }
        /** No move can possibly raise alpha. Prune this node. */
        if (stand_pat < alpha - big_delta) {
            return alpha;
        }
    }

    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    /** End block */

    CHECK_EVASIONS:
    for (size_t i = 0; i < n; ++i) {
        const move_t &candidate_move = moves[i];
        /** Delta pruning */
        if (!in_check && moveValue(candidate_move) + stand_pat < alpha - Weights::DELTA_MARGIN) {
            /** Skip evaluating this move */
            continue;
        }

        if (this->board.fastSEE(candidate_move) < 0) {
            continue;
        }

        this->pushMove(candidate_move);
        int32_t score = -qsearch(-beta, -alpha);
        this->popMove();
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
 * @brief Returns an integer moveValue, representing the score of the specified turn.
 *
 * @param alpha: Minimum score that the maximizing player is assured of.
 * @param beta: Maximum score that the minimizing player is assured of.
 */

#pragma ide diagnostic ignored "misc-no-recursion"

int32_t SearchContext::pvs(int16_t depth, int32_t alpha, int32_t beta, move_t *mv_hst) {
    if (!SearchContext::timeRemaining) {
        return 0;
    }

    TTEntry *t = transpositionTable.find(board.getHashCode());
    if (t != nullptr && t->depth >= depth) {
        switch (t->flag) {
            case EXACT:
                *mv_hst = t->best_move;
                return t->score;
            case LOWER:
                alpha = std::max(alpha, t->score);
                break;
            case UPPER:
                beta = std::min(beta, t->score);
                break;
        }

        if (alpha >= beta) {
            *mv_hst = t->best_move;
            return t->score;
        }
    }

    const int32_t original_alpha = alpha;
    if (depth <= 0) {
        // Extend the search_fd until the position is quiet
        return qsearch(alpha, beta);
    }

    // Move generation logic
    move_t mvs[MAX_MOVE_NUM];
    int n = this->board.genLegalMoves(mvs, this->board.getTurn());

    // Check lookahead terminating conditions
    if (n == 0) {
        if (board.isInCheck(board.getTurn())) {
            // King is in check, and there are no legal moves. Checkmate!
            return MATE_SCORE(ply);
        }
        // No legal moves, yet king is not in check. Stalemate!
        return 0;
    }
    if (isDrawn()) {
        return 0;
    }

    /** Move ordering logic */
    orderMoves(mvs, n);

    /** Begin PVS check first move */
    size_t pv_index = 0;
    move_t variations[depth + 1];

    this->pushMove(mvs[0]);
    variations[0] = mvs[0];
    int32_t mv_score = -pvs(depth - 1, -beta, -alpha, &variations[1]);
    this->popMove();

    if (mv_score > alpha) {
        alpha = mv_score;
        std::memcpy(mv_hst, variations, depth * sizeof(move_t));
    }

    if (alpha >= beta) {
        storeCutoffMove(mvs[0], depth);
        goto END;
    }
    /** End PVS check first move */

    /** PVS check subsequent moves */
    for (size_t i = 1; i < n; ++i) {
        const move_t &mv = mvs[i];
        /** Futility pruning */
        if (this->useFutilityPruning(mv, depth) && mv_score + this->moveValue(mv) < alpha - Weights::DELTA_MARGIN) {
            continue;
        }
        this->pushMove(mv);
        variations[0] = mv;

        int16_t r = this->computeReduction(mv, depth, i);
        /** Zero-Window Search. Assume good move ordering, and all subsequent mvs are worse. */
        mv_score = -1 * this->pvs(depth - 1 - r, -alpha - 1, -alpha, &variations[1]);
        /** If mvs[i] turns out to be better, re-search move with full window */
        if (alpha < mv_score && mv_score < beta) {
            mv_score = -1 * this->pvs(depth - 1, -beta, -alpha, &variations[1]);
        }
        this->popMove();

        if (mv_score > alpha) {
            alpha = mv_score;
            pv_index = i;
            std::memcpy(mv_hst, variations, depth * sizeof(move_t));
        }

        /** Beta-Cutoff */
        if (alpha >= beta) {
            storeCutoffMove(mv, depth);
            break;
        }
    }
    END:
    /** Updates the transposition table with the appropriate values */
    TTEntry tt_entry(this->board.getHashCode(), alpha, depth, EXACT, mvs[pv_index]);
    if (alpha <= original_alpha) {
        tt_entry.flag = UPPER;
    } else if (alpha >= beta) {
        tt_entry.flag = LOWER;
    }

    transpositionTable.insert(tt_entry);
    killerMoves[ply + 1].clear();
    return alpha;
}

void SearchContext::search_t() {
    move_t root_mvs[MAX_MOVE_NUM];
    int n_root_mvs = this->board.genLegalMoves(root_mvs, this->board.getTurn()); // TODO Refactor move gen

    pthread_mutex_lock(&init_lock);
    int seed = std::random_device()();
    std::mt19937 rng(seed);
    ++SearchContext::nActiveThreads;
    pthread_mutex_unlock(&init_lock);
    std::shuffle(root_mvs, &(root_mvs[n_root_mvs]), rng);

    move_t pv[MAX_DEPTH];

    const bool isMainThread = this->threadIndex == 0;
    for (int16_t d = 1; SearchContext::timeRemaining && d < MAX_DEPTH - 1; ++d) {
        int32_t evaluation = MIN_SCORE;

        for (int i = 0; i < n_root_mvs; ++i) {
            pv[0] = root_mvs[i];
            this->pushMove(pv[0]);
            int32_t mvScore = -1 * this->pvs(d - 1, MIN_SCORE, -evaluation, &pv[1]);
            this->popMove();

            if (mvScore > evaluation) {
                evaluation = mvScore;
                std::memcpy(this->threadPV, pv, d * sizeof(move_t));
            }
        }
        TTEntry ttEntry(board.getHashCode(), evaluation, d, EXACT, this->threadPV[0]);
        transpositionTable.insert(ttEntry);

        if (isMainThread && SearchContext::timeRemaining) {
            result.bestMove = this->threadPV[0];
            result.score = evaluation;
            SearchContext::timeManager.finishedIteration(evaluation);
        }
        orderMoves(root_mvs, n_root_mvs);
    }

    // Thread tear-down code
    if (isMainThread) {
        while (SearchContext::nActiveThreads > 1);
        pthread_mutex_destroy(&init_lock);
        SearchContext::timeManager.resetTimer();
    }
    --SearchContext::nActiveThreads;
    pthread_exit(nullptr);
}

SearchContext::SearchContext(const std::string &fen) : board(fen), position(&(this->board)) {
    pthread_mutex_init(&SearchContext::init_lock, nullptr);
    SearchContext::transpositionTable.initialize(strtol(uciInstance->getOption(option_t::hashSize).c_str(), nullptr, 10));

    std::memset(this->historyTable, 0, sizeof(int) * HTABLE_LEN);
    std::memset(this->threadPV, 0, MAX_DEPTH * sizeof(move_t));
    this->ply = 0;
    this->stack = nullptr;
    this->threadIndex = 0;
}

SearchContext::SearchContext(size_t threadIndex, const SearchContext &src) : position(&(this->board)) {
    this->board = src.board;

    std::unordered_map<uint64_t, RTEntry>::const_iterator it = src.repetitionTable.begin();
    const std::unordered_map<uint64_t, RTEntry>::const_iterator end = src.repetitionTable.end();

    while (it != end) {
        this->repetitionTable.insert(std::pair<uint64_t, RTEntry> (it->first, it->second));
        ++it;
    }

    std::memset(this->historyTable, 0, sizeof(int) * HTABLE_LEN);
    std::memset(this->threadPV, 0, MAX_DEPTH * sizeof(move_t));
    this->ply = 0;
    this->stack = nullptr;
    this->threadIndex = threadIndex;
}

void SearchContext::setUCIInstance(const UCI *uciPtr) {
    if (SearchContext::uciInstance) return;
    SearchContext::uciInstance = uciPtr;
}

SearchContext::~SearchContext() {}