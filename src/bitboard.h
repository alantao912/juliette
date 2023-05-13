#pragma once

#include <cstdint>

#include "util.h"

void initialize_zobrist();

void init_board(const char *fen);

void make_move(move_t move);

bool is_check(bool color);

bool is_move_check(move_t move);

bool is_attacked(bool color, int square);

uint64_t *get_bitboard(piece_t piece);

void print_bitboard(uint64_t bb);

void print_board();

move_t parse_move(const std::string &mv_str);