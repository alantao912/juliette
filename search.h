#pragma once

#include <unordered_set>
#include <vector>
#include <stack>
#include "util.h"

int32_t quiescence_search(uint16_t remaining_ply, int32_t alpha, int32_t beta, std::vector<move_t> *considered_line);

move_t search(uint16_t depth);

void showTopLine();