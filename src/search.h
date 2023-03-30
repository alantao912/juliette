#pragma once

#include <chrono>
#include <stack>
#include <vector>
#include <unordered_map>

#include "tables.h"
#include "uci.h"
#include "util.h"

namespace UCI {
    typedef struct info info_t;
};

typedef struct thread_args {
    const bitboard *main_board;
    const std::unordered_map<uint64_t, RTEntry> *main_repetition_table;
    move_t *root_mvs;
    int n_root_mvs;
} thread_args_t;

static bool is_drawn();

static bool verify_repetition(uint64_t hash);

static inline bool use_fprune(move_t cm, int16_t depth);

static int16_t reduction(int32_t score, int16_t current_ply);

static void order_moves(move_t mvs[], int32_t mv_scores[], int n);

static void store_cutoff_mv(move_t mv, int32_t mv_score);

static inline size_t h_table_index(const move_t &mv);

int32_t move_SEE(move_t move);

static int32_t SEE(int square);

static move_t find_lva(int square);

static int32_t piece_value(int square);

static int32_t move_value(move_t move);

static UCI::info_t generate_reply(int32_t evaluation, move_t best_move);

static void *__search(void *args);

UCI::info_t search(const int n_threads);

UCI::info_t search(int16_t depth);

UCI::info_t search(std::chrono::duration<int64_t, std::milli> time);
