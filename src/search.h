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

#define RECORD_TRACE

#ifdef RECORD_TRACE

struct TraceLogEntry 
{
    friend struct TraceLog;

private:
    // Index of parent trace log entry in the trace log
    int parentIndex;
    int iteration;
    int32_t score;
    move_t move;

public:
    TraceLogEntry(const move_t &, int, int);
};

struct TraceLog
{
    friend struct TraceLogEntry;

private:
    static const std::string outputPath;

    // Side-to-move of root position
    bool rootSTM;
    int finishedIterations;

    SearchStack<int> parentIndices;

    std::vector<TraceLogEntry> logEntries;

public:
    void flush();

    void pushEntry(const move_t &);

    void updateScore(int32_t);

    void initialize();

    void markNewParent();

    void eraseLastParent();
};

#endif

struct SearchContext
{

    friend struct UCI;

private:

    #ifdef RECORD_TRACE
    static TraceLog Log;
    #endif

    static const UCI *UciInstance;

    static struct info_t Result;

    static TimeManager TimeMan;

    static TTable TranspositionTable;

    static pthread_mutex_t ServiceLock;

    static const int32_t ContemptValue; // TODO: Initialize Later

    static volatile size_t NActiveThreads;

    static bool BlockHelpers;

    static const info_t &GetResult();

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

    static volatile bool TimeRemaining;

    static int32_t PieceValue(piece_t);

    static void SetUciInstance(const UCI *);

    SearchContext(const std::string &);

    SearchContext(size_t, const SearchContext &);

    ~SearchContext();

    void search_t();

    inline bool isMainThread();
};
