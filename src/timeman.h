//
// Created by Alan Tao on 3/23/2023.
//

#pragma once

#include <cstdint>
#include <vector>

struct TimeManager {

private:
    /** White time */
    int wTime;
    /** Black time */
    int bTime;

    /** White increment */
    int wInc;
    /** Black increment */
    int bInc;

    /** Moves to go until the next time control */
    int movesToGo;

    std::vector<int> duration;

    std::vector<int32_t> evaluations;
    int total;

public:

    void finished_iteration(int32_t evaluation);

    void initialize_timer(int wTime, int wInc, int bTime, int bInc, int movesToGo);

    void start_timer();

    void reset_timer();

};