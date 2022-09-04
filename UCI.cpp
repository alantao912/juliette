//
// Created by Alan Tao on 9/2/2022.
//

#include "UCI.h"

#define BUFLEN 512

std::map<std::string, std::string> options;
char *replies[] = {"id", "uciok", "readyok", "bestmove", "copyprotection", "registration", "info", "option"};

#define id 0
#define uciok 1
#define readyok 2
#define bestmove 3
#define copyprotection 4
#define registration 5
#define info 6
#define option 7

Board *game = nullptr;

/* Engine should use clientSocket to send reply to GUI */
SOCKET clientSocket;
char sendbuf[BUFLEN];

extern int source;

void initialize_options(SOCKET cs) {
    clientSocket = cs;
    options.insert(std::pair<std::string, std::string>("OwnBook", "off"));
    options.insert(std::pair<std::string, std::string>("debug", "off"));
}

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

    if (buff == "ucinewgame") {
        delete game;
        game = nullptr;
    } else if (buff == "position") {
        position(args);
    } else if (buff == "go") {
        go(args);
    } else if (buff == "quit") {
        std::cout << "juliette:: bye! i enjoyed playing with you :)" << std::endl;
        exit(0);
    }
}

std::vector<std::string> split(std::string &input) {
    std::vector<std::string> args;
    std::string tok;
    size_t p;

    while ((p = input.find(' ')) != std::string::npos) {
        tok = input.substr(0, p);
        args.push_back(tok);
        input.erase(0, p + 1);
    }
    return args;
}

void position(std::string &arg) {
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

void go(std::string &args) {
    std::vector<std::string> argv = split(args);
    // TODO: Configure function based on provided arguments according to UCI protocol.
    if (game == nullptr) {

        return;
    }
    uint32_t best_move = search(4);
    sprintf(sendbuf, "%s %c%d%c%d", replies[bestmove],
            GET_FROM_FILE(best_move) + 'a', GET_FROM_RANK(best_move), GET_TO_FILE(best_move) + 'a', GET_TO_RANK(best_move));
    reply();
}

void reply() {
    if (source == 0) {
        send(clientSocket, sendbuf, BUFLEN, 0);
    } else if (source == 1) {
        std::cout << sendbuf << std::endl;
    }
}