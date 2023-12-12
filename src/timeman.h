//
// Created by Alan Tao on 3/23/2023.
//

#pragma once

#include <cstdint>
#include <vector>

struct UCI;

void *timerThread(void *args);

struct TimerArgs {
    std::vector<int> *durationPtr;
    UCI *uciPtr;
};

struct TimeManager {

private:

    static UCI *uciInstance;

    int wTime;
    int bTime;

    int wIncrement;
    int bIncrement;

    // Moves remaining until next time control
    int movesToGo;

    std::vector<int> duration;
    std::vector<int32_t> evaluations;
    int total;

public:
    static void setUCIInstance(UCI *);

    void finishedIteration(int32_t);

    void initializeTimer(bool, int, int, int, int, int);

    void startTimer();

    void resetTimer();
};