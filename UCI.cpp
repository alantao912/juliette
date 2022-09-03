//
// Created by Alan Tao on 9/2/2022.
//

#include "UCI.h"

Board *game = nullptr;

void parse_UCI_string(char *uci) {
    std::string uci_string(uci), buff;
    size_t i = 0;

    while (i < uci_string.length() && uci_string.at(i) != ' ') {
        buff.push_back(uci_string.at(i));
        ++i;
    }
    /* Skips all of the spaces */
    while (i < uci_string.length() && uci_string.at(i) == ' ') {
        ++i;
    }
    std::string args;
    while (i < uci_string.length()) {
        args.push_back(uci_string.at(i++));
    }

    if (buff == "position") {
        position(args);
    } else if (buff == "ucinewgame") {
        delete game;
        game = nullptr;
    } else if (buff == "quit") {
        exit(0);
    }
}

void position(std::string arg) {
    if (arg == "startpos") {
        game = new Board(START_POSITION);
    } else {
        char *s = new char[arg.length() + 1];
        s[arg.length()] = 0;
        for (size_t i = 0; i < arg.length(); ++i) {
            s[i] = arg.at(i);
        }
        game = new Board(s);
        delete[] s;
    }
    initialize_evaluation();
    initialize_search();
}