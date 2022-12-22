#pragma once

#include <unordered_set>
#include <vector>
#include <stack>
#include "util.h"

typedef struct info {
    int32_t score;

    move_t best_move;
} info;

static inline bool is_drawn();

static inline bool verify_repetition(uint64_t hash);

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line);

info search(uint16_t depth);

void order_moves(move_t moves[], int n);
