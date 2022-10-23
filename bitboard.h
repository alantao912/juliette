#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

void initialize_zobrist();

void init_board(const char *fen);

void make_move(Move move);

bool is_check(bool color);

bool is_attacked(bool color, int square);

bool is_draw();
static bool _is_threefold_rep(void);
static bool _is_fifty_move_rule(void);

uint64_t* get_bitboard(char piece);

void print_board(void);