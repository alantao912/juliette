#include <algorithm>
#include <chrono>
#include <pthread.h>
#include <unordered_map>

#include "bitboard.h"
#include "stack.h"
#include "timeman.h"
#include "uci.h"
#include "util.h"

#define id 0
#define uciok 1
#define readyok 2
#define bestmove 3
#define copyprotection 4
#define registration 5
#define uci_info 6
#define option 7


const std::string UCI::idStr = "id name juliette author Alan Tao";
const std::string UCI::replies[8] = {"id", "uciok", "readyok", "bestmove", "copyprotection", "registration", "info_t", "option"};

void info_t::formatData(char buf[], size_t n, bool verbose) const {
    if (verbose) {
        std::string format("elapsed time: (%ld)ms\n%s:  %c%d%c%d\nevaluation: %d");
        // TODO: If mate score, format to M(n)
        snprintf(buf, BUFLEN, format.c_str(),
                 static_cast<long> (this->elapsedTime.count()), UCI::replies[bestmove].c_str(),
                 char(Bitboard::fileOf(this->bestMove.from) + 'a'),
                 int(Bitboard::rankOf(this->bestMove.from) + 1), char(Bitboard::fileOf(this->bestMove.to) + 'a'),
                 int(Bitboard::rankOf(this->bestMove.to) + 1), score);
    } else {
        snprintf(buf, BUFLEN, "%s: %c%d%c%d\n", UCI::replies[bestmove].c_str(),
                 char(Bitboard::fileOf(this->bestMove.from) + 'a'), int(Bitboard::rankOf(this->bestMove.from) + 1),
                 char(Bitboard::fileOf(this->bestMove.to) + 'a'), int(Bitboard::rankOf(this->bestMove.to) + 1));
    }
}

UCI::UCI() {}

void UCI::initializeUCI() {
    options.insert(std::pair<option_t, std::string>(option_t::ownBook, "off"));
    options.insert(std::pair<option_t, std::string>(option_t::debug, "off"));
    options.insert(std::pair<option_t, std::string>(option_t::threadCount, "14"));
    options.insert(std::pair<option_t, std::string>(option_t::contempt, "0"));
    options.insert(std::pair<option_t, std::string>(option_t::hashSize, "25165824"));
    TimeManager::setUCIInstance(this);
}

void UCI::parseUCIString(const char *uci) {
    std::string uci_string(uci);
    std::vector<std::string> tokens = StringUtils::split(uci_string);

    const std::string cmd = tokens[0];
    tokens.erase(tokens.begin(), tokens.begin() + 1);

    if (cmd == "uci") {
        this->initializeUCI();
        size_t len = UCI::idStr.size();
        std::memcpy(this->sendbuf, UCI::idStr.c_str(), len); // NOLINT(bugprone-not-null-terminated-result)
        this->sendbuf[len] = '\n';
        strcpy(&(this->sendbuf[len + 1]), replies[uciok].c_str());
        this->reply();
    } else if (cmd == "ucinewgame") {
        this->board_initialized = false;
        // TODO: Initialize zobrist hashes
        Bitboard::initializeZobrist();
    } else if (cmd == "isready") {
        snprintf(this->sendbuf, BUFLEN, "readyok");
        this->reply();
    } else if (cmd == "position") {
        UCI::position(tokens);
    } else if (cmd == "debug") {
        if (tokens[0] == "on" || tokens[0] == "off") {
            options[option_t::debug] = tokens[0];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: 'debug' option must be set to 'on' or 'off'");
            this->reply();
        }
    } else if (cmd == "go") {
        UCI::go(tokens);
    } else if (cmd == "setoption") {
        UCI::setOption(tokens);
    } else if (cmd == "stop") {
        SearchContext::timeRemaining = false;
        SearchContext::result.formatData(this->sendbuf, BUFLEN, this->options[option_t::debug] == "on");
        this->reply();
        UCI::joinThreads();
    } else if (cmd == "quit") {
        std::cout << "juliette:: bye! i enjoyed playing with you :)" << std::endl;
        UCI::joinThreads();
        exit(0);
    }
}

void UCI::position(const std::vector<std::string> &args) {
    if (this->mainThread) {
        delete this->mainThread;
        this->mainThread = nullptr;
    }

    size_t moves_index;
    if (args[0] == "startpos") {
        /** Position will be initialized from the starting position */
        this->mainThread = new SearchContext(START_POSITION);
        moves_index = 1;
    } else if (args.size() < 6) {
        snprintf(sendbuf, BUFLEN, "juliette:: Malformed FEN string");
        this->reply();
        return;
    } else {
        /** Recombine FEN that was split apart earlier */
        std::string fen;
        for (size_t i = 0; i < 4; ++i) {
            fen += args[i];
            fen += ' ';
        }
        this->mainThread = new SearchContext(fen);
        moves_index = 6;
    }
    bool b = true;
    for (std::vector<std::string>::const_iterator it = args.begin() + moves_index; it != args.end(); ++it) {
        move_t move = this->mainThread->board.parseMove(*it);
        if (move == NULL_MOVE) { b = false; break; }
        this->mainThread->pushMove(move);
    }
    if (!b) {
        snprintf(sendbuf, BUFLEN, "juliette:: board initialization failed!\n");
        this->reply();
    }
    board_initialized = b;
}

void UCI::go(const std::vector<std::string> &args) {
    if (!(this->board_initialized)) {
        snprintf(this->sendbuf, BUFLEN, "juliette:: a start position must be specified.");
        this->reply();
        return;
    }

    // Classical chess time control by default. 90 minutes, 30 second increment per move. 40 move time control
    int movesToGo = std::max(1, 40 - this->mainThread->board.fullmove_number);
    int wTime = 5400000;
    int bTime = 5400000;

    int wInc = 30000;
    int bInc = 30000;

    size_t index = 0;
    while (index < args.size()) {
        if (args[index] == "searchmoves") {
            // TODO implement after refactoring root shuffling
            index += 1;
        } else if (args[index] == "wtime") {
            if (!StringUtils::isNumber(&wTime, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "btime") {
            if (!StringUtils::isNumber(&bTime, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "winc") {
            if (!StringUtils::isNumber(&wInc, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "binc") {
            if (!StringUtils::isNumber(&bInc, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "movestogo") {
            if (!StringUtils::isNumber(&movesToGo, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "movetime") {
            int *variable = this->mainThread->board.fullmove_number ? &wTime : &bTime;
            if (!StringUtils::isNumber(variable, args[index + 1])) return;
            movesToGo = 1;
            wInc = 0;
            bInc = 0;
            index += 2;
        } else if (args[index] == "infinite" || args[index] == "ponder") {
            movesToGo = 1;
            wTime = INT32_MAX;
            bTime = INT32_MAX;
            wInc = INT32_MAX;
            bInc = INT32_MAX;
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette: '%s' token not supported.", args[index].c_str());
            this->reply();
            return;
        }
    }
    timeManager.initializeTimer(mainThread->board.getTurn(), wTime, wInc, bTime, bInc, movesToGo);
    this->nThreads = std::stoi(options[option_t::threadCount]);
    timeManager.startTimer();

    int status = pthread_create(&(this->threads[0]), nullptr, threadFunction, (void *) this->mainThread);
    if (status) {
        std::cout << "juliette:: Failed to spawn thread!\n";
        joinThreads();
        exit(-1);
    }
    for (int i = 1; i < nThreads; ++i) {
        status = pthread_create(&(this->threads[i]), nullptr, threadFunction, (void *) &(this->helperThreads[i - 1]));
        if (status) {
            std::cout << "juliette:: Failed to spawn thread!\n";
            joinThreads();
            exit(-1);
        }
    }
}

void UCI::formatData() {
    SearchContext::getResult().formatData(this->sendbuf, BUFLEN, this->options[option_t::debug] == "on");
}

void UCI::reply() {
    std::cout << this->sendbuf << std::endl;
}

void UCI::setOption(const std::vector<std::string> &args) {
    if (args[0] != "name" || args[2] != "value" || args.size() != 4) {
        snprintf(this->sendbuf, BUFLEN, "juliette:: syntax: setoption name [name] value [value]");
        this->reply();
        return;
    }

    if (args[1] == "contempt") {
        if (StringUtils::isNumber(args[3])) {
            options[option_t::contempt] = args[3];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: contempt value must be an integer");
            this->reply();
        }
    } else if (args[1] == "debug") {
        if (args[3] == "on" || args[3] == "off") {
            options[option_t::debug] = args[3];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: 'debug' option must be set to 'on' or 'off'");
            this->reply();
        }
    } else if (args[1] == "own_book") {
        if (args[3] == "on" || args[3] == "off") {
            options[option_t::ownBook] = args[3];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: 'own_book' option must be set to 'on' or 'off'");
            this->reply();
        }
    } else if (args[1] == "thread_cnt") {
        if (StringUtils::isNumber(args[3])) {
            options[option_t::threadCount] = args[3];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: thread count must be a number");
            this->reply();
        }
    } else if (args[1] == "hash_size") {
        if (StringUtils::isNumber(args[3])) {
            options[option_t::hashSize] = args[3];
        } else {
            snprintf(this->sendbuf, BUFLEN, "juliette:: hash size must be a number");
            this->reply();
        }
    } else {
        snprintf(this->sendbuf, BUFLEN, "juliette:: unrecognized option name '%s'", args[1].c_str());
        this->reply();
    }
}

const std::string &UCI::getOption(option_t opt) const {
    std::unordered_map<option_t, std::string>::const_iterator it = options.find(opt);
    return it->second;
}

void UCI::setElapsedTime(const std::chrono::milliseconds &elapsedTime) {
    SearchContext::result.elapsedTime = elapsedTime;
}

void UCI::joinThreads() {
    for (int i = 0; i < nThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    nThreads = 0;
}

void *threadFunction(void *arg) 
{
    SearchContext *context = reinterpret_cast<SearchContext *> (arg);
    context->search_t();
    return nullptr;
}