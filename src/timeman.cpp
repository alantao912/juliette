//
// Created by Alan Tao on 3/23/2023.
//

#include "timeman.h"
#include <chrono>
#include <iostream>
#include <pthread.h>
#include <thread>

extern volatile bool time_remaining;

static int duration;

static void *timer_thread(void *args) {
    time_remaining = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    time_remaining = false;
    pthread_exit(nullptr);
}

void start_timer(int ms) {
    pthread_t timer;
    duration = ms;
    int status = pthread_create(&timer, nullptr, timer_thread, (void *) &ms);
    if (status != 0) {
        std::cout << "juliette:: Failed to start timer thread\n";
        exit(-1);
    }
}