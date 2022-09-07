#pragma once

#include <unordered_set>
#include <stack>
#include <vector>
#include "Board.h"

void initialize_search();

uint32_t search(uint16_t depth);

void showTopLine();