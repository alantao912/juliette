//
// Created by Yvng Alan on 6/18/2023.
//

//
// Created by Yvng Alan on 6/17/2023.
//

#include "tables.h"
#include "util.h"

double TTable::load_factor = 0.33f;

TTEntry::TTEntry() {
    key = 0;
    initialized = false;
    flag = EXACT;
    score = 0;
    depth = 0;

    best_move = NULL_MOVE;
}

TTEntry::TTEntry(uint64_t hash_code, int32_t score, int16_t depth, flag_t flag, move_t best_move) {
    this->key = hash_code;

    this->score = score;
    this->depth = depth;
    this->flag = flag;
    this->best_move = best_move;
    initialized = true;
}

uint64_t TTEntry::operator()() const {
    return (key ^ depth ^ score ^ flag);
}

TTEntry &TTEntry::operator=(const TTEntry &other) {
    depth = other.depth;
    best_move = other.best_move;
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
    hash_full = false;
}

void TTable::initialize(std::size_t initial_capacity) {
    if (entries) {
        clear();
        hash_full = false;
    } else {
        entries = new TTEntry[initial_capacity];
        size = 0;
        capacity = initial_capacity;
        hash_full = false;
    }
}

TTable::~TTable() {
    delete[] entries;
}

void TTable::insert(const TTEntry &entry) {
    if (!hash_full) {
        hash_full = ((double) size / capacity) > load_factor;
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

TTEntry *TTable::get_backing_ptr() {
    return entries;
}

void TTable::clear() {
    size = 0;
    for (std::size_t i = 0; i < capacity; ++i) {
        entries[i].initialized = false;
    }
}

void test_transposition_table() {
    TTable table;
    table.initialize(1024);
    std::uint64_t hash_code = rand_bitstring();
    std::uint64_t start = hash_code;
    TTEntry entry(hash_code, 1, 5, EXACT, NULL_MOVE);
    table.insert(entry);

    hash_code += 1;
    TTEntry entry1(hash_code, 2, 3, UPPER, STALEMATE);
    table.insert(entry1);

    hash_code += 0;
    TTEntry entry2(hash_code, 6, 9, LOWER, CHECKMATE);
    table.insert(entry2);

    hash_code += 1024;
    TTEntry newEntry(hash_code, 7, 10, EXACT, NULL_MOVE);
    table.insert(newEntry);

    hash_code += 1024;
    TTEntry newEntry2(hash_code, 9, 2, LOWER, NULL_MOVE);
    table.insert(newEntry2);

    TTEntry *backing_ptr = table.get_backing_ptr();
    for (std::uint64_t i = 0; i < 10; ++i) {
        std::uint64_t index = (start % 1024) - 1 + i;
        if (backing_ptr[index].initialized) {
            std::cout << "[" << (unsigned long) (backing_ptr[index].key) << ", " << (int) backing_ptr[index].score << ", " << (int) backing_ptr[index].depth << "]";
        } else {
            std::cout << "[_]";
        }
    }
    std::cout << '\n';
    TTEntry *ptr = table.find(start + 1025);
    if (ptr) {
        std::cout << "Found\n";
        std::cout << ptr->score << ' ' << ptr->depth << '\n';
    } else {
        std::cout <<" Missing\n";
    }
}