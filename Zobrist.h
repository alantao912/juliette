//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>

#include "Board.h"

class Board;

void initialize_zobrist();

uint64_t rand_bitstring();

uint64_t fetch_bitstring(const Piece *p, const King *king);

uint64_t hash(const Board *game);

struct TTEntry {
    int32_t evaluation;
    uint16_t depth;
    uint8_t num_seen;

    TTEntry(int32_t e, uint16_t d, uint8_t n) : evaluation(e), depth(d), num_seen(n) {};
};

