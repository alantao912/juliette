#pragma once

#include "util.h"

void init_stack(void);

void push(move_t move);
void pop(void);

static void _free_stack();