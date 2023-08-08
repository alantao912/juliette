#include "timeman.h"
#include "uci.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <vector>

extern volatile bool time_remaining;
extern __thread bitboard board;

extern UCI::info_t result;

void *timer_thread(void *args) {
    time_remaining = true;
    size_t i = 0;
    std::vector<int> *duration_ptr = reinterpret_cast<std::vector<int> *> (args);
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    while (i < duration_ptr->size()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ptr->at(i++)));
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    time_remaining = false;
    result.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    UCI::format_data();
    UCI::reply();
    pthread_exit(nullptr);
    return nullptr;
}

void TimeManager::finished_iteration(int32_t evaluation) {
    if (!evaluations.empty()) {
        double numerator = 0.0, denominator = 0.0;
        for (size_t i = 0; i < evaluations.size(); ++i) {
            double weight = pow(double(i + 1) / double(total), 2.0);
            int32_t difference = evaluation - evaluations[i];
            numerator += (weight * difference);
            denominator += weight * evaluations[i];
        }
        double instability = numerator / denominator;
        if (instability > 1.5) {
            duration.push_back(this->wInc);
        }
    }
    evaluations.push_back(evaluation);
    total += evaluations.size();
}

void TimeManager::initialize_timer(int wT, int wI, int bT, int bI, int mTG) {
    this->wTime = wT;
    this->bTime = bT;

    this->wInc = wI;
    this->bInc = bI;

    this->movesToGo = mTG;
    if (board.turn) {
        duration.push_back(std::round((wTime + (this->movesToGo - 1) * wInc) / (float) (this->movesToGo)));
    } else {
        duration.push_back(std::round((bTime + (this->movesToGo - 1) * bInc) / (float) (this->movesToGo)));
    }
}

void TimeManager::start_timer() {
    pthread_t timer;
    int status = pthread_create(&timer, nullptr, timer_thread, &duration);
    if (status != 0) {
        std::cout << "juliette:: Failed to start timer thread\n";
        exit(-1);
    }
}

void TimeManager::reset_timer() {
    duration.clear();
    evaluations.clear();

    wTime = 0;
    wInc = 0;
    bTime = 0;
    bInc = 0;
    movesToGo = 0;

    total = 0;
}