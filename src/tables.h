//
// Created by Alan Tao on 9/4/2022.
//
#pragma once

#include <cstddef>
#include <cstdint>

#include "util.h"

enum BoundType {
    EXACT, LOWER, UPPER
};

struct TTEntry {
    uint64_t key;

    BoundType flag;

    int32_t score;
    int16_t depth;
    move_t bestMove;

    bool initialized;

    TTEntry();

    TTEntry(uint64_t hash_code, int32_t score, int16_t depth, BoundType flag, move_t best_move);

    uint64_t operator()() const;

    TTEntry &operator=(const TTEntry &other);
};

struct TTable {

    bool hashFull;

    TTable();

    ~TTable();

    void initialize(std::size_t initial_capacity);

    void insert(const TTEntry &entry);

    TTEntry *find(std::uint64_t hash_code);

    void clear();

private:

    static double loadFactor;

    std::size_t size, capacity;
    TTEntry *entries;
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

void test_transposition_table();