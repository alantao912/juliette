//
// Created by Alan Tao on 9/2/2022.
//

#pragma once

#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <cinttypes>
#include <windows.h>

#include "search.h"
#include "tables.h"
#include "evaluation.h"

void initialize_UCI(SOCKET clientSocket);

void parse_UCI_string(const char *uci);

std::vector<std::string> split(std::string &input);

void position(std::string &args);

void go(std::string &args);

void reply();
