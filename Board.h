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

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq --"

class King;
class Pawn;
class Piece;

class Board {
public:

    enum Color { WHITE, BLACK, EMPTY};

    enum Progression { OPENING, MIDDLEGAME, ENDGAME};

    Color move;

    Progression stage;

    Board(const char *fen);

    std::vector<uint32_t> *generate_moves();

    Piece *inspect(char file, char rank);

    char offset(char file, char rank);

    char offset_invert_rank(char file, char rank);

    std::vector<Piece *> *get_opposite_pieces(Color color);

    /**
     * @brief Get the pieces of the color it is to move.
     * 
     * @param color 
     * @return std::vector<Piece *>* 
     */

    std::vector<Piece *> *get_pieces_of_color(Color color);

    /**
     * @brief Gets the white pieces of the board.
     */

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

    uint64_t *squares_hit_by_pieces;

    Pawn *prev_jmp_pawn;

    /**
     * @brief Takes a vector of moves and takes out the moves that results in the king being in check.
     */

    void remove_illegal_moves(std::vector<uint32_t> *move_list);

    /**
     * @brief Given the amount of material on the board, determines whether the game is in the opening, middlegame, or endgame.
     * 
     * @return Progression. Either OPENING, MIDDLEGAME, or ENDGAME
     */

    Progression determine_game_stage();

    /**
     * @brief When reverting pawn promotion moves, we need to find the pawn containing the promoted piece, and delete its 'promoted_piece' 
     * field, and put the old pawn back on the board. This method finds the pawn that a given piece was promoted from.
     * 
     * @param promoted Piece that was promoted from a pawn.
     * @return Pawn* A pointer to the pawn that promoted to the given piece.
     */

    Pawn *find_parent_pawn(Piece *promoted);

    void print_rank(char rank);
};