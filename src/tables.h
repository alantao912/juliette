//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>
#include <cstdint>

#include "util.h"

enum flag_t {
    EXACT, LOWER, UPPER
};

struct TTEntry {
    flag_t flag;

    int32_t score;
    uint16_t depth;
    move_t best_move;

    bool is_pv;

    TTEntry(int32_t e, uint16_t d, flag_t f, move_t m) : score(e), depth(d), flag(f), best_move(m) {
        is_pv = false;
    };
};

struct RTEntry {

    uint8_t num_seen;

    explicit RTEntry(uint8_t n) : num_seen(n) {};

    RTEntry() {
        num_seen = 0;
    }

    RTEntry(const RTEntry &other) {
        num_seen = other.num_seen;
    }
};
