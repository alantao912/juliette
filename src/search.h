#pragma once

#include <chrono>
#include <stack>
#include <vector>

#include "uci.h"
#include "util.h"

namespace UCI {
    typedef struct info info_t;
};

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

UCI::info_t search(int16_t depth);

UCI::info_t search(std::chrono::duration<int64_t, std::milli> time);
