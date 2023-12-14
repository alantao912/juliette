//
// Created by Alan Tao on 6/18/2023.
//

#include "tables.h"
#include "util.h"

double TTable::loadFactor = 0.33f;

TTEntry::TTEntry() {
    key = 0;
    initialized = false;
    flag = EXACT;
    score = 0;
    depth = 0;

    bestMove = move_t::NULL_MOVE;
}

TTEntry::TTEntry(uint64_t hash_code, int32_t score, int16_t depth, BoundType flag, move_t best_move) {
    this->key = hash_code;

    this->score = score;
    this->depth = depth;
    this->flag = flag;
    this->bestMove = best_move;
    initialized = true;
}

uint64_t TTEntry::operator()() const {
    return (key ^ depth ^ score ^ flag);
}

TTEntry &TTEntry::operator=(const TTEntry &other) {
    depth = other.depth;
    bestMove = other.bestMove;
    score = other.score;
    flag = other.flag;
    key = other.key ^ depth ^ score ^ flag;
    initialized = true;
    return *this;
}

TTable::TTable() {
    entries = nullptr;
    size = 0;
    capacity = 0;
    hashFull = false;
}

void TTable::initialize(std::size_t initial_capacity) {
    if (entries) {
        clear();
        hashFull = false;
    } else {
        entries = new TTEntry[initial_capacity];
        size = 0;
        capacity = initial_capacity;
        hashFull = false;
    }
}

TTable::~TTable() {
    delete[] entries;
}

void TTable::insert(const TTEntry &entry) {
    if (!hashFull) {
        hashFull = ((double) size / capacity) > loadFactor;
    }

    for (std::size_t i = 0; i < capacity; ++i) {
        std::size_t index = (entry.key + i) % capacity;
        TTEntry &e = entries[index];

        if (e.initialized && entry.key != e()) {
            continue;
        }
        size += (!e.initialized);
        e = entry;
        return;
    }
}

TTEntry *TTable::find(std::uint64_t hash_code) {
    for (std::size_t i = 0, seen = 0; i < capacity && seen < size; ++i) {
        std::size_t index = (hash_code + i) % capacity;
        TTEntry &e = entries[index];
        if (!e.initialized) {
            return nullptr;
        } else if (hash_code == e()) {
            return &entries[index];
        }
        ++seen;
    }
    return nullptr;
}

void TTable::clear() {
    size = 0;
    for (std::size_t i = 0; i < capacity; ++i) {
        entries[i].initialized = false;
    }
}