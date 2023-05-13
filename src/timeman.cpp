//
// Created by Alan Tao on 3/23/2023.
//

#include "timeman.h"
#include <unistd.h>

extern bool time_remaining;

void *start_timer(void *args) {
    int *t = (int *) static_cast<int *> (args);
    time_remaining = true;
    sleep(*t);
    time_remaining = false;
    return nullptr;
}
