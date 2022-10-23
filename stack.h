#pragma once

#include "util.h"

void init_stack(void);

void push(Move move);
void pop(void);

static void _free_stack();