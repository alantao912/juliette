#pragma once

#include <unordered_set>
#include <chrono>
#include <vector>
#include <stack>
#include "util.h"

typedef struct info {
    int32_t score;

    move_t best_move;
} info_t;

static bool is_drawn();

static bool verify_repetition(uint64_t hash);

static inline bool use_fprune(move_t cm, int16_t depth);

static int16_t reduction(int16_t score, int16_t current_ply);

static void order_moves(move_t moves[], int n);

static void insert_killer_move(move_t mv);

int16_t move_SEE(move_t move);

static int16_t SEE(int square);

static move_t find_lva(int square);

static int16_t piece_value(int square);

static int16_t move_value(move_t move);

static info_t generate_reply(int32_t evaluation, move_t best_move);

info_t search(int16_t depth);

info_t search(std::chrono::duration<int64_t, std::milli> time);
