#pragma once

#define A_FILE 0
#define B_FILE 1
#define C_FILE 2
#define D_FILE 3
#define F_FILE 5
#define G_FILE 6
#define H_FILE 7

#include <map>
#include <stack>
#include <vector>
#include <iostream>

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq --"

#define WHITE_KNIGHT 0
#define WHITE_BISHOP 1
#define WHITE_ROOK 2
#define WHITE_QUEEN 3
#define WHITE_PAWN 4

#define BLACK_KNIGHT 5
#define BLACK_BISHOP 6
#define BLACK_ROOK 7
#define BLACK_QUEEN 8
#define BLACK_PAWN 9

class King;
class Pawn;
class Piece;

class Board {
public:

    enum Color { WHITE, BLACK, EMPTY};

    enum Progression { OPENING, MIDDLEGAME, ENDGAME};

    /* Stores the color of the pieces turn to move. */

    Color move;

    /* 64 byte hash for the board's state */

    char position_hash[64];

    /**
     * @brief Indicates whether the state of the board is in the opening, middle-game, or endgame.
     */

    Progression stage;

    /**
     * @brief Construct a new Board object with a given FEN string. Does not count 50 move rule.
     */

    Board(const char *fen);

    /**
     * @brief Generates a list of legal moves for the color it is to move. See Piece.h to see how moves are encoded in a 32 bit integer.
     */

    std::vector<uint32_t> *generate_moves();

    /**
     * @brief Returns a pointer to the piece stored at the specified file and rank. If no piece is present, returns nullptr.
     */

    Piece *inspect(int8_t file, int8_t rank);

    /**
     * @brief Returns an offset to the array index of specified file and rank.
     */

    int8_t offset(int8_t file, int8_t rank);

    /**
     * @brief Returns an offset to the array index of specified file and rank, except
     * the rank is inverted. Effectively rotates the board, and returns the array index of rotated board.
     */

    int8_t offset_invert_rank(int8_t file, int8_t rank);

    /**
     * @brief Get the pieces of the opposite color specified.
     * 
     * @param color 
     * @return std::vector<Piece *>* list of pieces opposite to the color specified.
     */

    std::vector<Piece *> *get_opposite_pieces(Color color);

    /**
     * @brief Get the pieces of the color it is to move.
     */

    std::vector<Piece *> *get_pieces_of_color(Color color);

    /**
     * @brief Gets the white pieces on the board.
     */

    std::vector<Piece *> *get_white_pieces();

    /**
     * @brief Get the black pieces on the board.
     */

    std::vector<Piece *> *get_black_pieces();

    /**
     * @brief Returns the collection of the given pieces.
     */

    std::vector<Piece *> *get_collection(Color color, uint8_t type);

    /**
     * @brief Get the king of the specified color
     */

    King *get_my_king(Color color);

    /**
     * @brief Gets the king of the opposite color specified.
     */

    King *get_opponent_king(Color color);

    size_t game_depth();

    /* Prints the board using ASCII characters to stdout */

    void print_board();

    /* Prints a given move */

    void print_move(uint32_t move);

    /* Modifies the state of the board according to the move specified, and stores the move onto a stack. */

    void make_move(uint32_t move);

    /* Reverts the state of the board to prior the last move on the move stack was made. Pops a move off of the stack. */
    uint32_t revert_move();

    bool is_king_in_check();

private:
    std::stack<uint32_t> move_stack;
    std::stack<Piece *> captured_pieces;

    std::vector<Piece *> black_pieces;
    std::vector<Piece *> white_pieces;

    /* Stores all of the pieces of the same type into one vector. i.e. all black pawns are grouped together, all white knights are grouped together etc. */

    std::vector<Piece *> *pieces_collections[10];

    King *white_king, *black_king;

    Piece *squares[64];

    /* Pointer to pawn that last jumped two squares. */

    Pawn *prev_jmp_pawn;

    /**
     * @brief Takes a vector of moves and takes out the moves that results in the king being in check.
     * Orders the moves putting checks first, captures second, then quiet moves.
     */

    void filter_moves(std::vector<uint32_t> *move_list);

    /**
     * @brief Removes a given piece from the specified collection. If piece is not found in collection, collection is left unchanged. 
     */

    void remove_from_collection(std::vector<Piece *> *collection, Piece *p);

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

    /**
     * @brief Prints out a single row of the board. 
     */

    void print_rank(int8_t rank);
};