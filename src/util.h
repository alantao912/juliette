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
enum Squares 
{
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
enum MoveFlags 
{
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

enum piece_t 
{
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    EMPTY
};

/**
 * Representation of a move.
 */
struct move_t 
{

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

    int32_t normalizeScore() const;

    void setScore(type_t, int32_t);

    bool isType(type_t) const;

    bool operator==(const move_t &) const;

    bool operator<(const move_t &) const;

    std::string to_string() const;
};

extern const uint64_t BB_KNIGHT_ATTACKS[64];

extern const move_t NULL_MOVE;
extern const move_t CHECKMATE;
extern const move_t STALEMATE;
extern const int MAX_MOVE_NUM;
extern const int MAX_CAPTURE_NUM;
extern const int MAX_ATTACK_NUM;

namespace BitUtils 
{
    uint64_t getRandomBitstring();

    int getLSB(uint64_t);

    int pullLSB(uint64_t *);

    int popCount(uint64_t);

    uint64_t flipBitboardVertical(uint64_t);

    uint64_t _get_reverse_bb(uint64_t);

    void setBit(uint64_t *, int);

    void clearBit(uint64_t *, int);
}

namespace ConversionUtils 
{
    piece_t toEnum(char);

    char toChar(const piece_t &);

    std::string moveToString(const move_t &);
}

namespace StringUtils
{
    void ltrim(std::string &);

    void rtrim(std::string &);

    void trim(std::string &);

    bool isNumber(const std::string &);

    bool isNumber(int *, const std::string &);

    std::vector<std::string> split(std::string &);
}


namespace IOUtils 
{
    int parseSquare(const char *);

    void printMove(move_t);

    void printBitstring32(const uint32_t);

    void printBitboard(uint64_t);
}