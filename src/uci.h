//
// Created by Alan Tao on 9/2/2022.
//

#pragma once

#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <cinttypes>

#include "search.h"
#include "tables.h"

namespace UCI {

    enum option_t {
        contempt, debug, own_book, thread_cnt, hash_size
    };

    typedef struct info {
        int32_t score;

        move_t best_move;
        std::chrono::milliseconds elapsed_time;

        void format_data(bool verbose) const;
    } info_t;

    void initialize_UCI();

    void parse_UCI_string(const char *uci);

    void position(const std::vector<std::string> &args);

    void go(const std::vector<std::string> &args);

    void format_data();

    void reply();

    void set_option(const std::vector<std::string> &args);

    const std::string &get_option(UCI::option_t);
};
