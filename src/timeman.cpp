//
// Created by Alan Tao on 3/23/2023.
//

#include "timeman.h"
#include "uci.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <vector>

extern volatile bool time_remaining;

void *timer_thread(void *args) {
    time_remaining = true;
    size_t i = 0;
    std::vector<int> *duration_ptr = (std::vector<int> *) args;
    while (i < duration_ptr->size()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ptr->at(i++)));
    }
    time_remaining = false;
    UCI::format_data();
    UCI::reply();
    pthread_exit(nullptr);
    return nullptr;
}

void TimeManager::finished_iteration(int32_t evaluation) {
    if (!evaluations.empty()) {
        /** TODO: Using the history of evaluations, and time remaining, determine whether more time needs to be added. */
        double numerator = 0.0, denominator = 0.0;
        for (size_t i = 0; i < evaluations.size(); ++i) {
            double weight = pow(double(i + 1) / double(total), 2.0);
            int32_t difference = evaluation - evaluations[i];
            numerator += (weight * difference);
            denominator += weight * evaluations[i];
        }
        double instability = numerator / denominator;
        //std::cout << "Instability: " << instability << '\n';
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
    duration.push_back(std::round((wTime + (mTG - 1) * wInc) / (float) (this->movesToGo)));
    std::cout << duration[0] << '\n';
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