#include "timeman.h"
#include "uci.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <vector>

UCI *TimeManager::uciInstance;

void *timerThread(void *args) {
    size_t i = 0;
    TimerArgs *timerArgs = reinterpret_cast<TimerArgs *> (args);

    SearchContext::timeRemaining = true;
    std::vector<int> *durationPtr = timerArgs->durationPtr;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    while (i < durationPtr->size()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(durationPtr->at(i++)));
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    SearchContext::timeRemaining = false;
    std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    timerArgs->uciPtr->setElapsedTime(elapsedTime);
    timerArgs->uciPtr->formatData();
    timerArgs->uciPtr->reply();
    timerArgs->uciPtr->joinThreads();
    pthread_exit(nullptr);
    return nullptr;
}

void TimeManager::setUCIInstance(UCI *uciInstance) {
    TimeManager::uciInstance = uciInstance;
}

void TimeManager::finishedIteration(int32_t evaluation) {
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
            duration.push_back(this->wIncrement);
        }
    }
    evaluations.push_back(evaluation);
    total += evaluations.size();
}

void TimeManager::initializeTimer(bool sideTomove, int wTime, int wIncrement, int bTime, int bIncrement, int movesToGo) {
    this->wTime = wTime;
    this->bTime = bTime;

    this->wIncrement = wIncrement;
    this->bIncrement = bIncrement;

    this->movesToGo = movesToGo;
    if (sideTomove) {
        duration.push_back(std::round((wTime + (this->movesToGo - 1) * wIncrement) / (float) (this->movesToGo)));
    } else {
        duration.push_back(std::round((bTime + (this->movesToGo - 1) * bIncrement) / (float) (this->movesToGo)));
    }
}

void TimeManager::startTimer() {
    pthread_t timer;
    TimerArgs args = {.durationPtr = &(this->duration), .uciPtr = TimeManager::uciInstance};
    int status = pthread_create(&timer, nullptr, timerThread, &args);
    if (status) {
        std::cout << "juliette:: Failed to start timer thread\n";
        exit(-1);
    }
}

void TimeManager::resetTimer() {
    duration.clear();
    evaluations.clear();

    wTime = 0;
    wIncrement = 0;
    bTime = 0;
    bIncrement = 0;
    movesToGo = 0;

    total = 0;
}