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

#include "Search.h"
#include "tables.h"
#include "Evaluation.h"

void initialize_UCI(SOCKET clientSocket);

void parse_UCI_string(char *uci);

std::vector<std::string> split(std::string &input);

void position(std::string &args);

void go(std::string &args);

void reply();
