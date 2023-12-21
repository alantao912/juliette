#pragma once

#include <chrono>
#include <pthread.h>
#include <stack>
#include <unordered_map>
#include <vector>

#include "bitboard.h"
#include "evaluation.h"
#include "stack.h"
#include "tables.h"
#include "timeman.h"
#include "uci.h"
#include "util.h"

#define MAX_DEPTH 128

struct SearchContext
{

    friend struct UCI;

private:
    static const UCI *uciInstance;

    static struct info_t result;

    static TimeManager timeManager;

    static TTable transpositionTable;

    static pthread_mutex_t serviceLock;

    static const int32_t contempt_value; // TODO: Initialize Later

    static volatile size_t nActiveThreads;

    static bool blockHelpers;

    static const info_t &getResult();

    std::unordered_map<uint64_t, RTEntry> repetitionTable;

    Evaluation position;

    std::vector<move_t> killerMoves[MAX_DEPTH];

    move_t threadPV[MAX_DEPTH];

    int32_t historyTable[HTABLE_LEN];

    Bitboard board;

    LinkedStack *stack;

    int16_t ply;

    size_t threadIndex;

    int16_t computeReduction(move_t, int16_t, int);

    bool verifyRepetition(uint64_t);

    bool isDrawn();

    bool useFutilityPruning(move_t, int16_t);

    size_t hTableIndex(const move_t &);

    int32_t moveValue(move_t);

    void storeCutoffMove(move_t, int16_t);

    void orderMoves(move_t *, int);

    void pushMove(const move_t &);

    void popMove();

    int32_t qsearch(int32_t, int32_t);

    int32_t pvs(int16_t, int32_t, int32_t, move_t *);

public:

    static volatile bool timeRemaining;

    static int32_t pieceValue(piece_t);

    static void setUCIInstance(const UCI *);

    SearchContext(const std::string &);

    SearchContext(size_t threadIndex, const SearchContext &src);

    ~SearchContext();

    void search_t();
};
