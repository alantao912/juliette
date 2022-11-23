//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>
#include <cstdint>

enum flag_t {EXACT, LOWER, UPPER};

struct TTEntry {
    flag_t flag;

    int32_t evaluation;
    uint16_t depth;

    TTEntry(int32_t e, uint16_t d, flag_t f) : evaluation(e), depth(d), flag(f) {
    };
};

struct RTEntry {
    uint8_t num_seen;

    explicit RTEntry(uint8_t n) : num_seen(n) {};
};
