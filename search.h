#pragma once

#include <unordered_set>
#include <stack>
#include <vector>
#include "util.h"

int32_t quiescence_search(int32_t alpha, int32_t beta, std::vector<Move> *considered_line);

Move search(uint16_t depth);

void showTopLine();