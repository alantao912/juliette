//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>
#include <cstdint>

struct TTEntry {
    int32_t evaluation;
    uint16_t depth;
    move_t best_move;
    bool searched;

    TTEntry(int32_t e, uint16_t d) : evaluation(e), depth(d) {
        searched = false;
    };
};

struct RTEntry {
    uint8_t num_seen;

    explicit RTEntry(uint8_t n) : num_seen(n) {};
};
