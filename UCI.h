//
// Created by Alan Tao on 9/2/2022.
//

#pragma once

#include <vector>
#include <cstring>
#include <iostream>
#include <cinttypes>

#include "Board.h"
#include "Search.h"
#include "Evaluation.h"

void parse_UCI_string(char *uci);

void position(std::string args);
