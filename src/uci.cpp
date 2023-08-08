#include <algorithm>
#include <chrono>
#include <pthread.h>
#include <unordered_map>

#include "bitboard.h"
#include "stack.h"
#include "timeman.h"
#include "uci.h"
#include "util.h"

#define BUFLEN 512

std::map<UCI::option_t, std::string> options;

const char *id_str = "id name juliette author Alan Tao";
std::string replies[] = {"id", "uciok", "readyok", "bestmove", "copyprotection", "registration", "info_t", "option"};

#define id 0
#define uciok 1
#define readyok 2
#define bestmove 3
#define copyprotection 4
#define registration 5
#define uci_info 6
#define option 7

extern __thread bitboard board;
extern thread_local std::unordered_map<uint64_t, RTEntry> repetition_table;
extern bool time_remaining;

bool board_initialized = false;
char sendbuf[BUFLEN];

UCI::info_t result;
TimeManager timeManager;

thread_args_t main_arg;
thread_args_t aux_args;

void UCI::info_t::format_data(bool verbose) const {
    if (verbose) {
        std::string format("elapsed time: (%ld)ms\n%s:  %c%d%c%d\nevaluation: %d");
        // TODO: If mate score, format to M(n)
        snprintf(sendbuf, BUFLEN, format.c_str(),
                 static_cast<long> (elapsed_time.count()), replies[bestmove].c_str(),
                 char(file_of(best_move.from) + 'a'),
                 int(rank_of(best_move.from) + 1), char(file_of(best_move.to) + 'a'),
                 int(rank_of(best_move.to) + 1), score);
    } else {
        snprintf(sendbuf, BUFLEN, "%s: %c%d%c%d\n", replies[bestmove].c_str(),
                 char(file_of(best_move.from) + 'a'), int(rank_of(best_move.from) + 1),
                 char(file_of(best_move.to) + 'a'), int(rank_of(best_move.to) + 1));
    }
}

void UCI::initialize_UCI() {
    options.insert(std::pair<UCI::option_t, std::string>(UCI::option_t::own_book, "off"));
    options.insert(std::pair<UCI::option_t, std::string>(UCI::option_t::debug, "on"));
    options.insert(std::pair<UCI::option_t, std::string>(UCI::option_t::thread_cnt, "14"));
    options.insert(std::pair<UCI::option_t, std::string>(UCI::option_t::contempt, "0"));
    options.insert(std::pair<UCI::option_t, std::string>(UCI::option_t::hash_size, "25165824"));
}

void UCI::parse_UCI_string(const char *uci) {
    std::string uci_string(uci);
    std::vector<std::string> tokens = split(uci_string);

    const std::string cmd = tokens[0];
    tokens.erase(tokens.begin(), tokens.begin() + 1);

    if (cmd == "uci") {
        initialize_UCI();
        size_t len = strlen(id_str);
        memcpy(sendbuf, id_str, len); // NOLINT(bugprone-not-null-terminated-result)
        sendbuf[len] = '\n';
        strcpy(&sendbuf[len + 1], replies[uciok].c_str());
        UCI::reply();
    } else if (cmd == "ucinewgame") {
        board_initialized = false;
        initialize_zobrist();
    } else if (cmd == "position") {
        UCI::position(tokens);
    } else if (cmd == "go") {
        UCI::go(tokens);
    } else if (cmd == "setoption") {
        UCI::set_option(tokens);
    } else if (cmd == "quit") {
        std::cout << "juliette:: bye! i enjoyed playing with you :)" << std::endl;
        exit(0);
    }
}

void UCI::position(const std::vector<std::string> &args) {
    size_t moves_index;
    if (args[0] == "startpos") {
        /** Position will be initialized from the starting position */
        init_board(START_POSITION);
        moves_index = 1;
    } else if (args.size() < 6) {
        snprintf(sendbuf, BUFLEN, "juliette:: Malformed FEN string");
        UCI::reply();
        return;
    } else {
        /** Recombine FEN that was split apart earlier */
        std::string fen;
        for (size_t i = 0; i < 4; ++i) {
            fen += args[i];
            fen += ' ';
        }
        init_board(fen.c_str());
        try {
            board.halfmove_clock = std::stoi(args[4]);
            board.fullmove_number = std::stoi(args[5]);
        } catch (const std::invalid_argument &e) {
            snprintf(sendbuf, BUFLEN, "juliette:: Move counters must be integers");
            UCI::reply();
            return;
        }
        moves_index = 6;
    }
    bool b = std::all_of(args.begin() + moves_index, args.end(),
                         [](const std::string &arg) {
                             move_t mv = parse_move(arg);
                             if (mv == NULL_MOVE) {
                                 return false;
                             }
                             push(mv);
                             return true;
                         });
    if (!b) {
        std::cout << "juliette:: board initialization failed!\n";
    }
    board_initialized = b;
}

bool UCI::validate_integer_argument(int *variable, std::string arg) {
    if (!is_number(arg)) {
        snprintf(sendbuf, BUFLEN, "juliette: token following %s must be an integer.", arg.c_str());
        UCI::reply();
        return false;
    }
    *variable = std::stoi(arg);
    return true;
}

void UCI::go(const std::vector<std::string> &args) {
    if (!board_initialized) {
        snprintf(sendbuf, BUFLEN, "juliette:: a start position must be specified.");
        UCI::reply();
        return;
    }

    // Classical chess time control by default. 90 minutes, 30 second increment per move. 40 move time control
    int movesToGo = 40;
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
            if (!validate_integer_argument(&wTime, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "btime") {
            if (!validate_integer_argument(&bTime, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "winc") {
            if (!validate_integer_argument(&wInc, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "binc") {
            if (!validate_integer_argument(&bInc, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "movestogo") {
            if (!validate_integer_argument(&movesToGo, args[index + 1])) return;
            index += 2;
        } else if (args[index] == "movetime") {
            int *variable = nullptr;
            if (board.turn) {
                variable = &wTime;
            } else {
                variable = &bTime;
            }
            if (!validate_integer_argument(variable, args[index + 1])) return;
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
            snprintf(sendbuf, BUFLEN, "juliette: '%s' token not supported.", args[index].c_str());
            UCI::reply();
            return;
        }
    }
    timeManager.initialize_timer(wTime, wInc, bTime, bInc, movesToGo);
    int n_threads = stoi(options[UCI::option_t::thread_cnt]);
    pthread_t threads[n_threads];
    main_arg = {.main_board = &board, .main_repetition_table = &repetition_table, .is_main_thread = true};
    aux_args = {.main_board = &board, .main_repetition_table = &repetition_table, .is_main_thread = false};

    timeManager.start_timer();

    int status = pthread_create(&threads[0], nullptr, reinterpret_cast<void *(*)(void *)> (search_t),
                                (void *) &main_arg);
    for (int i = 1; i < n_threads; ++i) {
        status |= pthread_create(&threads[i], nullptr, reinterpret_cast<void *(*)(void *)> (search_t),
                                 (void *) &aux_args);
    }
    if (status != 0) {
        std::cout << "juliette:: Failed to spawn thread!\n";
        exit(-1);
    }
}

void UCI::format_data() {
    result.format_data(options[UCI::option_t::debug] == "on");
}

void UCI::reply() {
    std::cout << sendbuf << std::endl;
}

void UCI::set_option(const std::vector<std::string> &args) {
    if (args[0] != "name" || args[2] != "value" || args.size() != 4) {
        snprintf(sendbuf, BUFLEN, "juliette:: syntax: setoption name [name] value [value]");
        UCI::reply();
        return;
    }

    if (args[1] == "contempt") {
        if (is_number(args[3])) {
            options[UCI::option_t::contempt] = args[3];
        } else {
            snprintf(sendbuf, BUFLEN, "juliette:: contempt value must be an integer");
            UCI::reply();
        }
    } else if (args[1] == "debug") {
        if (args[3] == "on" || args[3] == "off") {
            options[UCI::option_t::debug] = args[3];
        } else {
            snprintf(sendbuf, BUFLEN, "juliette:: 'debug' option must be set to 'on' or 'off'");
            UCI::reply();
        }
    } else if (args[1] == "own_book") {
        if (args[3] == "on" || args[3] == "off") {
            options[UCI::option_t::own_book] = args[3];
        } else {
            snprintf(sendbuf, BUFLEN, "juliette:: 'own_book' option must be set to 'on' or 'off'");
            UCI::reply();
        }
    } else if (args[1] == "thread_cnt") {
        if (is_number(args[3])) {
            options[UCI::option_t::thread_cnt] = args[3];
        } else {
            snprintf(sendbuf, BUFLEN, "juliette:: thread count must be a number");
            UCI::reply();
        }
    } else if (args[1] == "hash_size") {
        if (is_number(args[3])) {
            options[UCI::option_t::hash_size] = args[3];
        } else {
            snprintf(sendbuf, BUFLEN, "juliette:: hash size must be a number");
            UCI::reply();
        }
    } else {
        snprintf(sendbuf, BUFLEN, "juliette:: unrecognized option name '%s'", args[1].c_str());
        UCI::reply();
    }
}

const std::string &UCI::get_option(UCI::option_t opt) {
    return options[opt];
}