#pragma once

#include <unordered_set>
#include <vector>
#include <stack>
#include "util.h"

typedef struct info {
    int32_t score;

    move_t best_move;
} info_t;

static inline bool is_drawn();

static inline bool verify_repetition(uint64_t hash);

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta);

int16_t reduction(int16_t score, int16_t current_ply);
void order_moves(move_t moves[], int n);

int move_SEE(move_t move);
int SEE(int square);
move_t find_lva(int square);
int value(int square);
int value(move_t move);

info_t search(int16_t depth);
