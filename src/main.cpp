#include <iostream>
#include <iomanip>
#include <pthread.h>

#include "uci.h"
#include "stack.h"
#include "movegen.h"
#include "bitboard.h"
#include "weights.h"
#include "evaluation.h"

#define BUFLEN 512

extern __thread bitboard board;

void play_game() {
    init_board(START_POSITION);
    initialize_zobrist();
    std::string user_input;
    int depth;
    do {
        std::cout << "juliette:: Engine depth? ";
        try {
            std::cin >> user_input;
            trim(user_input);
            depth = std::stoi(user_input);
        } catch (const std::invalid_argument &arg) {
            depth = -1;
        }
    } while (depth <= 0);

    do {
        std::cout << "juliette:: Player Color? (w/b): ";
        std::cin >> user_input;
        trim(user_input);
    } while (user_input != "w" && user_input != "b");

    UCI::info_t reply;
    std::chrono::steady_clock::time_point start, end;
    if (user_input == "b") {
        start = std::chrono::steady_clock::now();
        reply = search_fd((int16_t) depth);
        end = std::chrono::steady_clock::now();
        push(reply.best_move);
    }

    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    bool player_turn = true;
    while (n) {
        // system("clear");
        std::cout << "Elapsed Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << '\n';
        print_board();
        if (player_turn) {
            int i;
            for (i = 0; i < n; ++i) {
                std::cout << (i + 1) << ' ';
                print_move(moves[i]);
                std::cout << '\n';
            }

            do {
                std::cout << "juliette:: move? ";
                try {
                    std::cin >> user_input;
                    trim(user_input);
                    i = std::stoi(user_input);
                    if (i == 0) {
                        std::cout << "Exiting\n";
                        exit(0);
                    }
                } catch (const std::invalid_argument &arg) {
                    i = -1;
                }
            } while (i < 1 || i > n);
            push(moves[i - 1]);
        } else {
            start = std::chrono::steady_clock::now();
            reply = search_fd((int16_t) depth);
            end = std::chrono::steady_clock::now();
            push(reply.best_move);
        }
        player_turn = !player_turn;
        n = gen_legal_moves(moves, board.turn);
    }
    if (is_check(board.turn)) {
        std::cout << "juliette:: Checkmate, ";
        if (player_turn) {
            std::cout << "computer wins!\n";
        } else {
            std::cout << "player wins!\n";
        }
    } else {
        std::cout << "juliette:: Stalemate, the game is drawn.\n";
    }
}

/**
 * To compile: g++ src/*.cpp -lpthread -o juliette
 *
 * ./juliette cli
 */

int main(int argc, char *argv[]) {
    init_bishop_attacks();
    init_rook_attacks();
    _init_rays();
    if (argc == 1) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* If no options are provided */
        play_game();
        return 0;
    }

    char recvbuf[BUFLEN];
    enum comm_mode {
        UNDEFINED, UCI
    };
    comm_mode communication_mode = UNDEFINED;

    if (strcmp(argv[1], "cli") == 0) {
        std::cout << "juliette:: \"hi, let's play chess!\"" << std::endl;
        /* Engine is set to CLI mode. Sending and receiving commands through stdout and stdin respectively. */
        do {
            std::cin.getline(recvbuf, BUFLEN);
            if ((strlen(recvbuf) > 0) && (recvbuf[strlen(recvbuf) - 1] == '\n')) {
                recvbuf[strlen(recvbuf) - 1] = '\0';
            }
            if (communication_mode == UCI) {
                UCI::parse_UCI_string(recvbuf);
            } else if (strcmp(recvbuf, "uci") == 0) {
                communication_mode = UCI;
                UCI::initialize_UCI();
                UCI::parse_UCI_string(recvbuf);
            } else if (strcmp(recvbuf, "comm") == 0) {
                std::cout << "juliette:: to select a communication protocol, enter it's name:" << std::endl;
                std::cout << "uci" << std::endl;
                std::cout << "dev (developer use)" << std::endl;
                std::cout << "perft" << std::endl;
            } else if (strcmp(recvbuf, "dev") == 0) {
                std::cout << "juliette:: switched to development mode." << std::endl;
                /**
                 * To be used for development purposes:
                 */

                initialize_zobrist();
                init_board("6k1/8/8/2r5/4p3/3P4/2B5/2K5 b - - 0 1");

                move_t moves[MAX_MOVE_NUM];
                int n = gen_legal_moves(moves, board.turn);
                for (int i = 0; i < n; ++i) {
                    std::cout << i + 1 << ". ";
                    print_move(moves[i]);
                    std::cout << '\n';
                }
                int m;
                std::cin >> m;
                int32_t score = fast_SEE(moves[m - 1]);
                std::cout << "SEE score: " << score << '\n';
                //test_transposition_table();
            } else if (strcmp(recvbuf, "perft") == 0) {
                std::cout << "juliette:: starting performance test..." << std::endl;
                // TODO: Re-implement performance test
            } else if (strlen(recvbuf)) {
                std::cout
                        << R"(juliette:: communication format not set, type "uci" to specify UCI communication protocol or type "comm" to see a list of communication protocol.)"
                        << std::endl;
            }
        } while (strlen(recvbuf));
    } else if (strcmp(argv[1], "tune") == 0) {
        initialize_zobrist();
        init_board(START_POSITION);

        char *move_seq;
        if (argc < 3) {
            move_seq = new char;
            *move_seq = (char) 0;
        } else {
            move_seq = argv[2];
        }
        const std::size_t len = strlen(move_seq);
        std::size_t i = 0;

        while (i < len) {
            move_t moves[MAX_MOVE_NUM];
            int n = gen_legal_moves(moves, board.turn);

            const char src_file = move_seq[i] - 'a';
            const char src_rank = move_seq[i + 1] - '1';

            const char dest_file = move_seq[i + 2] - 'a';
            const char dest_rank = move_seq[i + 3] - '1';

            const char prom = move_seq[i + 4];
            int j;
            for (j = 0; j < n; ++j) {
                move_t mv = moves[j];
                if (8 * src_rank + src_file != mv.from || 8 * dest_rank + dest_file != mv.to) {
                    continue;
                }

                if (prom == 'q' && (mv.flag == PR_QUEEN || mv.flag == PC_QUEEN)) {
                    push(mv);
                    break;
                } else if (prom == 'r' && (mv.flag == PR_ROOK || mv.flag == PC_ROOK)) {
                    push(mv);
                    break;
                } else if (prom == 'b' && (mv.flag == PR_BISHOP || mv.flag == PC_BISHOP)) {
                    push(mv);
                    break;
                } else if (prom == 'n' && (mv.flag == PR_KNIGHT || mv.flag == PC_KNIGHT)) {
                    push(mv);
                    break;
                } else if (prom == 0 || prom == ' ') {
                    push(mv);
                    break;
                }
            }
            if (j == n) {
                std::cout << "Move: [";
                std::cout << (char) (src_file + 'a') << (int) (src_rank + 1) << (char) (dest_file + 'a')
                          << (int) (dest_rank + 1);
                std::cout << "] not found. ";
                std::cout << i << std::endl;
                return 0;
            }
            i += 5;
        }
        UCI::info_t reply = search_fd((int16_t) 2);
        print_move(reply.best_move);
    }
    return 0;
}