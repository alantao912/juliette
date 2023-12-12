#pragma once

#include <cstdint>

#include "util.h"

struct Bitboard {

    friend struct Evaluation;
    friend struct UCI;

private:
    piece_t mailbox[64]; // piece-centric board representation

    uint64_t wPawns;
    uint64_t wKnights;
    uint64_t wBishops;
    uint64_t wRooks;
    uint64_t wQueens;
    uint64_t wKing;
    uint64_t bPawns;
    uint64_t bKnights;
    uint64_t bBishops;
    uint64_t bRooks;
    uint64_t bQueens;
    uint64_t bKing;

    uint64_t occupied;
    uint64_t wOccupied;
    uint64_t bOccupied;

    int wKingSquare;
    int bKingSquare;

    bool turn;

    bool wKingsideCastleRights;
    bool wQueensideCastleRights;
    bool bKingsideCastleRights;
    bool bQueensideCastleRights;

    // en passant target square, if any
    int en_passant_square;

    // number of halfmoves since the last capture or pawn advance
    int halfmove_clock;
    // number of cycles of a white move and a black move
    int fullmove_number;

    // hash code for the current position
    uint64_t hash_code;

    // Internal helper functions for movegen

    /**
     * @param color
     * @return the bitboard of squares the king of the color can't go.
     * All squares the color is attacking.
     */
    uint64_t _get_attackmask(bool color);

    /**
     * @param color the color of the king possibly in check.
     * @return all squares if the king is not in check, else
     * the path between the attacking piece and the color's king.
     * Returns empty bitboard if in double or more check.
     */
    uint64_t _get_checkmask(bool color);

    /**
     * @param color
     * @param king_square
     * @param square the square the possibly pinned piece is on.
     * @return a possible pin ray for the piece.
     */
    uint64_t _get_pinmask(bool color, int square);

    /**
     * @param color the side to move
     * @param piece
     * @param from the square the piece is moving from
     * @param to the square the piece is moving to
     * @return the appropriate flag for the move, excludes promotions
     */
    MoveFlags getFlag(piece_t piece, int from, int to);

    /**
     * @param color the color of the king
     * @param square the square the king is on
     * @return where the king can move from the given square
     */
    uint64_t get_king_moves(bool color, int square);

    uint64_t get_king_moves_no_castle(bool color, int square);

    /**
     * @param color the color of the queen
     * @param square the square the queen is on
     * @return where the queen can move from the given square
     */
    uint64_t get_queen_moves(bool color, int square);

    /**
     * @param color the color of the rook
     * @param square the square the rook is on
     * @return where the rook can move from the given square
     */
    uint64_t get_rook_moves(bool color, int square);

    /**
     * @param color the color of the bishop
     * @param square the square the bishop is on
     * @return where the bishop can move from the given square
     */
    uint64_t get_bishop_moves(bool color, int square);

    /**
     * @param color the color of the knight
     * @param square the square the knight is on
     * @return where the knight can move from the given square
     */
    uint64_t get_knight_moves(bool color, int square);

    /**
     * @param color the color of the pawn.
     * @param square the square the pawn is on.
     * @return where the pawn can move from the given square.
     */
    uint64_t get_pawn_moves(bool color, int square);

    // Internal helper functions for fastSEE

    uint64_t findLVA(uint64_t, piece_t &, uint64_t);

    uint64_t considerXrayAttacks(int, int);

    void clearBb(uint64_t);

public:

    // Bitboard constants

    static const uint64_t BB_SQUARES[64];

    static const uint64_t BB_ALL;

    static const uint64_t BB_FILE_A;
    static const uint64_t BB_FILE_B;
    static const uint64_t BB_FILE_C;
    static const uint64_t BB_FILE_D;
    static const uint64_t BB_FILE_E;
    static const uint64_t BB_FILE_F;
    static const uint64_t BB_FILE_G;
    static const uint64_t BB_FILE_H;
    static const uint64_t BB_FILES[8];

    static const uint64_t BB_RANK_1;
    static const uint64_t BB_RANK_2;
    static const uint64_t BB_RANK_3;
    static const uint64_t BB_RANK_4;
    static const uint64_t BB_RANK_5;
    static const uint64_t BB_RANK_6;
    static const uint64_t BB_RANK_7;
    static const uint64_t BB_RANK_8;
    static const uint64_t BB_RANKS[8];

    // Numbered from lower right to upper left

    static const uint64_t BB_DIAGONAL_1;
    static const uint64_t BB_DIAGONAL_2;
    static const uint64_t BB_DIAGONAL_3;
    static const uint64_t BB_DIAGONAL_4;
    static const uint64_t BB_DIAGONAL_5;
    static const uint64_t BB_DIAGONAL_6;
    static const uint64_t BB_DIAGONAL_7;
    static const uint64_t BB_DIAGONAL_8;
    static const uint64_t BB_DIAGONAL_9;
    static const uint64_t BB_DIAGONAL_10;
    static const uint64_t BB_DIAGONAL_11;
    static const uint64_t BB_DIAGONAL_12;
    static const uint64_t BB_DIAGONAL_13;
    static const uint64_t BB_DIAGONAL_14;
    static const uint64_t BB_DIAGONAL_15;
    static const uint64_t BB_DIAGONALS[15];

    // Numbered from lower left to upper right

    static const uint64_t BB_ANTI_DIAGONAL_1;
    static const uint64_t BB_ANTI_DIAGONAL_2;
    static const uint64_t BB_ANTI_DIAGONAL_3;
    static const uint64_t BB_ANTI_DIAGONAL_4;
    static const uint64_t BB_ANTI_DIAGONAL_5;
    static const uint64_t BB_ANTI_DIAGONAL_6;
    static const uint64_t BB_ANTI_DIAGONAL_7;
    static const uint64_t BB_ANTI_DIAGONAL_8;
    static const uint64_t BB_ANTI_DIAGONAL_9;
    static const uint64_t BB_ANTI_DIAGONAL_10;
    static const uint64_t BB_ANTI_DIAGONAL_11;
    static const uint64_t BB_ANTI_DIAGONAL_12;
    static const uint64_t BB_ANTI_DIAGONAL_13;
    static const uint64_t BB_ANTI_DIAGONAL_14;
    static const uint64_t BB_ANTI_DIAGONAL_15;
    static const uint64_t BB_ANTI_DIAGONALS[15];


    static uint64_t BB_RAYS[64][64];

    static uint64_t ZOBRIST_VALUES[781];

    static const int MAX_MOVE_NUM;
    static const int MAX_CAPTURE_NUM;
    static const int MAX_ATTACK_NUM;

    // Static bitboard utility functions

    static int rankOf(int);

    static int fileOf(int);

    static int diagonalOf(int);

    static int antiDiagonalOf(int);

    static uint64_t getRayBetween(int, int);

    static uint64_t getRayBetweenInclusive(int, int);

    // Initialize Structures

    static void initializeZobrist();

    // Bitboard instance methods

    Bitboard(const std::string &);

    Bitboard();

    /**
     * Takes in an empty array and generates the list of legal moves in it.
     * @param moves the array to store the moves in.
     * @param color the side to move.
     * @param return the number of moves.
     */
    int genLegalMoves(move_t *moves, bool color);

    /**
     * Takes in an empty array and generates the list of legal captures in it.
     * @param moves the array to store the captures in.
     * @param color the side to move.
     * @param return the number of captures.
     */
    int genLegalCaptures(move_t *moves, bool color);

    int genNonquiescentMoves(move_t *moves, bool color);

    void makeMove(const move_t &);

    move_t parseMove(const std::string &);

    bool isInCheck(bool);

    bool isMoveCheck(const move_t &);

    bool isAttacked(bool, int);

    bool containsPromotions();

    int32_t pieceValue(int);

    int32_t fastSEE(move_t);

    uint64_t attacksTo(int);

    uint64_t *getBitboard(const piece_t &);

    void printBoard();

    uint64_t getHashCode();

    int getHalfmoveClock();

    bool getTurn() const;

    piece_t lookupMailbox(int);

    // overloaded operators

    bool operator==(const Bitboard &); // TODO: Implement

    void operator=(const Bitboard &);
};