#pragma once

#include <chrono>
#include <iostream>
#include <vector>

#define WHITE 1
#define BLACK 0
#define INVALID (-1)
#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"
#define HTABLE_LEN 768

/**
 * Index enumeration of individual squares on the chess board.
 */
enum squares {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};


/**
 * Special characteristics of a move.
 */
enum move_flags {
    NONE, // No special flag
    PASS, // Null move
    CASTLING,
    EN_PASSANT,
    CAPTURE,
    PR_KNIGHT,
    PR_BISHOP,
    PR_ROOK,
    PR_QUEEN,
    PC_KNIGHT, // Promotion that is also a capture
    PC_BISHOP,
    PC_ROOK,
    PC_QUEEN
};

/**
 * Indexed enumeration of piece types.
 */

enum piece_t {
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    EMPTY
};

/**
 * Representation of a move.
 */
typedef struct move_t {

    static const int32_t SCORE_MASK = 0xFFFFFF;

    enum type_t {
        LOSING_EXCHANGE, QUIET, KILLER_MOVE, WINNING_EXCHANGE, CHECK_MOVE, HASH_MOVE
    };

    unsigned int from: 6;
    unsigned int to: 6;
    unsigned int flag: 4;

    /**
     * Most significant 8-bits of the following integer are used to disambiguate the move "type".
     * More significant bits have higher precedence in move ordering. Lower 24 bits used to differentiate
     * moves of the same type.
     *
     * MSB                            LSB
     * ________  ________________________
     *  Type                       Score
     */
    int32_t score;

    int32_t normalize_score() const;

    void set_score(type_t t, int32_t score);

    bool is_type(type_t t) const;

    bool operator==(const move_t &other) const;

    bool operator<(const move_t &other) const;

    std::string to_string() const;
} move_t;

typedef struct bitboard {
    piece_t mailbox[64]; // piece-centric board representation

    uint64_t w_pawns;
    uint64_t w_knights;
    uint64_t w_bishops;
    uint64_t w_rooks;
    uint64_t w_queens;
    uint64_t w_king;
    uint64_t b_pawns;
    uint64_t b_knights;
    uint64_t b_bishops;
    uint64_t b_rooks;
    uint64_t b_queens;
    uint64_t b_king;

    uint64_t occupied;
    uint64_t w_occupied;
    uint64_t b_occupied;

    int w_king_square;
    int b_king_square;

    bool turn;

    bool w_kingside_castling_rights;
    bool w_queenside_castling_rights;
    bool b_kingside_castling_rights;
    bool b_queenside_castling_rights;

    int en_passant_square; // en passant target square, if any

    int halfmove_clock; // number of halfmoves since the last capture or pawn advance
    int fullmove_number; // number of cycles of a white move and a black move

    uint64_t hash_code; // hash_code hash move_value for the current position
} bitboard;

/**
 * A stack of all the board positions that's been reached and
 * the moves that got to them.
 */
typedef struct stack_t {
    bitboard board;
    struct stack_t *next;

    move_t prev_mv;
} stack_t;

extern const uint64_t BB_KNIGHT_ATTACKS[64];

extern const uint64_t BB_SQUARES[64];

extern const uint64_t BB_ALL;

extern const uint64_t BB_FILE_A;
extern const uint64_t BB_FILE_B;
extern const uint64_t BB_FILE_C;
extern const uint64_t BB_FILE_D;
extern const uint64_t BB_FILE_E;
extern const uint64_t BB_FILE_F;
extern const uint64_t BB_FILE_G;
extern const uint64_t BB_FILE_H;
extern const uint64_t BB_FILES[8];

extern const uint64_t BB_RANK_1;
extern const uint64_t BB_RANK_2;
extern const uint64_t BB_RANK_3;
extern const uint64_t BB_RANK_4;
extern const uint64_t BB_RANK_5;
extern const uint64_t BB_RANK_6;
extern const uint64_t BB_RANK_7;
extern const uint64_t BB_RANK_8;
extern const uint64_t BB_RANKS[8];

extern const uint64_t BB_DIAGONAL_1;
extern const uint64_t BB_DIAGONAL_2;
extern const uint64_t BB_DIAGONAL_3;
extern const uint64_t BB_DIAGONAL_4;
extern const uint64_t BB_DIAGONAL_5;
extern const uint64_t BB_DIAGONAL_6;
extern const uint64_t BB_DIAGONAL_7;
extern const uint64_t BB_DIAGONAL_8;
extern const uint64_t BB_DIAGONAL_9;
extern const uint64_t BB_DIAGONAL_10;
extern const uint64_t BB_DIAGONAL_11;
extern const uint64_t BB_DIAGONAL_12;
extern const uint64_t BB_DIAGONAL_13;
extern const uint64_t BB_DIAGONAL_14;
extern const uint64_t BB_DIAGONAL_15;
extern const uint64_t BB_DIAGONALS[15];

extern const uint64_t BB_ANTI_DIAGONAL_1;
extern const uint64_t BB_ANTI_DIAGONAL_2;
extern const uint64_t BB_ANTI_DIAGONAL_3;
extern const uint64_t BB_ANTI_DIAGONAL_4;
extern const uint64_t BB_ANTI_DIAGONAL_5;
extern const uint64_t BB_ANTI_DIAGONAL_6;
extern const uint64_t BB_ANTI_DIAGONAL_7;
extern const uint64_t BB_ANTI_DIAGONAL_8;
extern const uint64_t BB_ANTI_DIAGONAL_9;
extern const uint64_t BB_ANTI_DIAGONAL_10;
extern const uint64_t BB_ANTI_DIAGONAL_11;
extern const uint64_t BB_ANTI_DIAGONAL_12;
extern const uint64_t BB_ANTI_DIAGONAL_13;
extern const uint64_t BB_ANTI_DIAGONAL_14;
extern const uint64_t BB_ANTI_DIAGONAL_15;
extern const uint64_t BB_ANTI_DIAGONALS[15];

extern uint64_t BB_RAYS[64][64];

extern uint64_t ZOBRIST_VALUES[781];

extern const move_t NULL_MOVE;
extern const move_t CHECKMATE;
extern const move_t STALEMATE;
extern const int MAX_MOVE_NUM;
extern const int MAX_CAPTURE_NUM;
extern const int MAX_ATTACK_NUM;

piece_t to_enum(char p);

char to_char(piece_t p);

uint64_t get_ray_between(int square1, int square2);

uint64_t get_ray_between_inclusive(int square1, int square2);

uint64_t vflip_bb(uint64_t);

int get_lsb(uint64_t bb);

void set_bit(uint64_t *bb, int square);

void clear_bit(uint64_t *bb, int square);

int pop_count(uint64_t bb);

int rank_of(int square);

int file_of(int square);

int diagonal_of(int square);

int anti_diagonal_of(int square);

int pull_lsb(uint64_t *bb);

int parse_square(const char *square);

void print_move(move_t move);

void print_bitstring32(const uint32_t);

void ltrim(std::string &string);

void rtrim(std::string &string);

void trim(std::string &string);

std::vector<std::string> split(std::string &input);

bool is_number(const std::string &input);