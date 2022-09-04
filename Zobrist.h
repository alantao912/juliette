//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>

#include "King.h"
#include "Pawn.h"
#include "Board.h"
#include "Piece.h"

struct ZobristHashFunction {
private:
    static uint64_t rand_bitstring();

public:

    static void initialize();

    size_t operator()(const Board *game);
};

struct ZobristHashComparator {

    bool operator==(const size_t i);
};

struct TTEntry {
    int32_t evaluation;
    uint16_t depth;
    uint8_t num_seen;

    TTEntry(int32_t e, uint16_t d, uint8_t n) : evaluation(e), depth(d), num_seen(n) {};
};

