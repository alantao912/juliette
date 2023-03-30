//
// Created by Alan Tao on 3/23/2023.
//

#include "timeman.h"
#include <iostream>
#include <unistd.h>

extern bool time_remaining;

void *start_timer(void *args) {
    int *t = (int *) static_cast<int *> (args);
    time_remaining = true;
    sleep(*t);
    time_remaining = false;
    if (!time_remaining) {
        std::cout << "time remaining set to false\n";
    }
    return nullptr;
}
