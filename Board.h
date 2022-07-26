#pragma once

#define A_FILE 0
#define B_FILE 1
#define C_FILE 2
#define D_FILE 3
#define E_FILE 4
#define F_FILE 5
#define G_FILE 6
#define H_FILE 7

#include <map>
#include <stack>
#include <vector>
#include <iostream>

class King;
class Pawn;
class Piece;

class Board {
public:

    enum Color {WHITE, BLACK, EMPTY};

    Color move;

    Board(const char *fen);

    Board();

    std::vector<uint32_t> *generate_moves();

    Piece *inspect(char file, char rank);

    std::vector<Piece *> *get_opposite_pieces(Color color);

    std::vector<Piece *> *get_current_pieces();

    std::vector<Piece *> *get_white_pieces();

    std::vector<Piece *> *get_black_pieces();

    King *get_my_king(Color color);

    void print_board();

    void print_move(uint32_t move);

    void make_move(uint32_t move);

    uint32_t revert_move();

private:
    std::stack<uint32_t> move_stack;
    std::stack<Piece *> captured_pieces;

    std::vector<Piece *> black_pieces;
    std::vector<Piece *> white_pieces;

    King *white_king, *black_king;

    Piece *squares[64];

    Pawn *prev_jmp_pawn;

    void remove_illegal_moves(std::vector<uint32_t> *move_list);

    Pawn *find_parent_pawn(Piece *promoted);

    char offset(char file, char rank);

    void print_rank(char rank);
};