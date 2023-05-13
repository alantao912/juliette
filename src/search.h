#pragma once

#include <chrono>
#include <stack>
#include <vector>
#include <unordered_map>

#include "tables.h"
#include "uci.h"
#include "util.h"

namespace UCI {
    typedef struct info info_t;
};

typedef struct thread_args {
    const bitboard *main_board;
    const std::unordered_map<uint64_t, RTEntry> *main_repetition_table;

    bool is_main_thread;

} thread_args_t;

int32_t move_SEE(move_t move);

void search_t(thread_args_t *args);

UCI::info_t search_fd(int16_t depth);

UCI::info_t search_ft(std::chrono::duration<int64_t, std::milli> time);
