#include <algorithm>

#include "util.h"
#include "search.h"
#include "bitboard.h"
#include "weights.h"

const uint64_t BB_SQUARES[64] = {
        1ULL << A1, 1ULL << B1, 1ULL << C1, 1ULL << D1, 1ULL << E1, 1ULL << F1, 1ULL << G1, 1ULL << H1,
        1ULL << A2, 1ULL << B2, 1ULL << C2, 1ULL << D2, 1ULL << E2, 1ULL << F2, 1ULL << G2, 1ULL << H2,
        1ULL << A3, 1ULL << B3, 1ULL << C3, 1ULL << D3, 1ULL << E3, 1ULL << F3, 1ULL << G3, 1ULL << H3,
        1ULL << A4, 1ULL << B4, 1ULL << C4, 1ULL << D4, 1ULL << E4, 1ULL << F4, 1ULL << G4, 1ULL << H4,
        1ULL << A5, 1ULL << B5, 1ULL << C5, 1ULL << D5, 1ULL << E5, 1ULL << F5, 1ULL << G5, 1ULL << H5,
        1ULL << A6, 1ULL << B6, 1ULL << C6, 1ULL << D6, 1ULL << E6, 1ULL << F6, 1ULL << G6, 1ULL << H6,
        1ULL << A7, 1ULL << B7, 1ULL << C7, 1ULL << D7, 1ULL << E7, 1ULL << F7, 1ULL << G7, 1ULL << H7,
        1ULL << A8, 1ULL << B8, 1ULL << C8, 1ULL << D8, 1ULL << E8, 1ULL << F8, 1ULL << G8, 1ULL << H8
};

const uint64_t BB_ALL = 0xffffffffffffffff;

const uint64_t BB_FILE_A = 0x0101010101010101;
const uint64_t BB_FILE_B = BB_FILE_A << 1;
const uint64_t BB_FILE_C = BB_FILE_A << 2;
const uint64_t BB_FILE_D = BB_FILE_A << 3;
const uint64_t BB_FILE_E = BB_FILE_A << 4;
const uint64_t BB_FILE_F = BB_FILE_A << 5;
const uint64_t BB_FILE_G = BB_FILE_A << 6;
const uint64_t BB_FILE_H = BB_FILE_A << 7;
const uint64_t BB_FILES[8] = {BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F, BB_FILE_G, BB_FILE_H};

const uint64_t BB_RANK_1 = 0xff;
const uint64_t BB_RANK_2 = BB_RANK_1 << 8;
const uint64_t BB_RANK_3 = BB_RANK_1 << 16;
const uint64_t BB_RANK_4 = BB_RANK_1 << 24;
const uint64_t BB_RANK_5 = BB_RANK_1 << 32;
const uint64_t BB_RANK_6 = BB_RANK_1 << 40;
const uint64_t BB_RANK_7 = BB_RANK_1 << 48;
const uint64_t BB_RANK_8 = BB_RANK_1 << 56;
const uint64_t BB_RANKS[8] = {BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8};

const uint64_t BB_DIAGONAL_1 = 0x80; // Numbered from lower right to upper left
const uint64_t BB_DIAGONAL_2 = 0x8040;
const uint64_t BB_DIAGONAL_3 = 0x804020;
const uint64_t BB_DIAGONAL_4 = 0x80402010;
const uint64_t BB_DIAGONAL_5 = 0x8040201008;
const uint64_t BB_DIAGONAL_6 = 0x804020100804;
const uint64_t BB_DIAGONAL_7 = 0x80402010080402;
const uint64_t BB_DIAGONAL_8 = 0x8040201008040201;
const uint64_t BB_DIAGONAL_9 = 0x4020100804020100;
const uint64_t BB_DIAGONAL_10 = 0x2010080402010000;
const uint64_t BB_DIAGONAL_11 = 0x1008040201000000;
const uint64_t BB_DIAGONAL_12 = 0x804020100000000;
const uint64_t BB_DIAGONAL_13 = 0x402010000000000;
const uint64_t BB_DIAGONAL_14 = 0x201000000000000;
const uint64_t BB_DIAGONAL_15 = 0x100000000000000;
const uint64_t BB_DIAGONALS[15] = {BB_DIAGONAL_1, BB_DIAGONAL_2, BB_DIAGONAL_3, BB_DIAGONAL_4, BB_DIAGONAL_5, BB_DIAGONAL_6,
                                   BB_DIAGONAL_7, BB_DIAGONAL_8, BB_DIAGONAL_9, BB_DIAGONAL_10, BB_DIAGONAL_11, BB_DIAGONAL_12,
                                   BB_DIAGONAL_13, BB_DIAGONAL_14, BB_DIAGONAL_15};

const uint64_t BB_ANTI_DIAGONAL_1 = 0x1; // Numbered from lower left to upper right
const uint64_t BB_ANTI_DIAGONAL_2 = 0x102;
const uint64_t BB_ANTI_DIAGONAL_3 = 0x10204;
const uint64_t BB_ANTI_DIAGONAL_4 = 0x1020408;
const uint64_t BB_ANTI_DIAGONAL_5 = 0x102040810;
const uint64_t BB_ANTI_DIAGONAL_6 = 0x10204081020;
const uint64_t BB_ANTI_DIAGONAL_7 = 0x1020408102040;
const uint64_t BB_ANTI_DIAGONAL_8 = 0x102040810204080;
const uint64_t BB_ANTI_DIAGONAL_9 = 0x204081020408000;
const uint64_t BB_ANTI_DIAGONAL_10 = 0x408102040800000;
const uint64_t BB_ANTI_DIAGONAL_11 = 0x810204080000000;
const uint64_t BB_ANTI_DIAGONAL_12 = 0x1020408000000000;
const uint64_t BB_ANTI_DIAGONAL_13 = 0x2040800000000000;
const uint64_t BB_ANTI_DIAGONAL_14 = 0x4080000000000000;
const uint64_t BB_ANTI_DIAGONAL_15 = 0x8000000000000000;
const uint64_t BB_ANTI_DIAGONALS[15] = {BB_ANTI_DIAGONAL_1, BB_ANTI_DIAGONAL_2, BB_ANTI_DIAGONAL_3, BB_ANTI_DIAGONAL_4, BB_ANTI_DIAGONAL_5, BB_ANTI_DIAGONAL_6,
                                        BB_ANTI_DIAGONAL_7, BB_ANTI_DIAGONAL_8, BB_ANTI_DIAGONAL_9, BB_ANTI_DIAGONAL_10, BB_ANTI_DIAGONAL_11, BB_ANTI_DIAGONAL_12,
                                        BB_ANTI_DIAGONAL_13, BB_ANTI_DIAGONAL_14, BB_ANTI_DIAGONAL_15};

uint64_t BB_RAYS[64][64];

uint64_t ZOBRIST_VALUES[781];

extern bitboard board;

const move_t NULL_MOVE = {A1, A1, PASS};
const move_t CHECKMATE = NULL_MOVE;
const move_t STALEMATE = {H8, H8, PASS};

/** Maximum number of legal moves in a given position */
const int MAX_MOVE_NUM = 218;
/** Maximum number of legal captures in a given position */
const int MAX_CAPTURE_NUM = 74;
/** Maximum number of attacks on a single square */
const int MAX_ATTACK_NUM = 16;
/** Score added to a move's "interestingness" if it gives check. */
const int CHECK_SCORE = 20000;

bool move_t::operator <(const move_t &other) const {
    return score > other.score;
};

void move_t::compute_score() {
    score = 0;
    if (is_move_check(*this)) {
        score += CHECK_SCORE;
    }
    switch (flag) {
        case PC_QUEEN:
            score += QUEEN_MATERIAL + (int16_t) piece_value(to);
            break;
        case PC_ROOK:
            score += ROOK_MATERIAL + (int16_t) piece_value(to);
            break;
        case PC_BISHOP:
            score += BISHOP_MATERIAL + (int16_t) piece_value(to);
            break;
        case PC_KNIGHT:
            score += KNIGHT_MATERIAL + (int16_t) piece_value(to);
            break;
        case PR_QUEEN:
            score += QUEEN_MATERIAL;
            break;
        case PR_ROOK:
            score += ROOK_MATERIAL;
            break;
        case PR_BISHOP:
            score += BISHOP_MATERIAL;
            break;
        case PR_KNIGHT:
            score += KNIGHT_MATERIAL;
            break;
        case CASTLING:
            score += 100;
            break;
        case CAPTURE:
            if (piece_value(from) < piece_value(to)) {
                score += piece_value(to) - piece_value(from);
                break;
            }
        case NONE:
            score += move_SEE(*this);
            break;
        case EN_PASSANT:
        default:
            break;
    }
}

uint64_t get_ray_between(int square1, int square2) {
    return (BB_RAYS[square1][square2] & ((BB_ALL << square1) ^ (BB_ALL << square2))) | BB_SQUARES[square2];
}

uint64_t get_ray_between_inclusive(int square1, int square2) {
    return BB_RAYS[square1][square2];
}

int get_lsb(uint64_t bb) {
    return __builtin_ffsll(bb) - 1;
}

/**
 * Turn on the square on the bitboard.
 * @param bb the bitboard.
 * @param square
 */
void set_bit(uint64_t *bb, int square) {
    *bb |= (1ULL << square);
}

/**
 * Clear the square on the bitboard.
 * @param bb the bitboard.
 * @param square
 */
void clear_bit(uint64_t* bb, int square) {
    *bb &= ~(1ULL << square);
}

int pop_count(uint64_t bb) {
    return __builtin_popcountll(bb);
}

/**
 * @param square
 * @return the rank of the square (0-7)
 */
int rank_of(int square) {
    return square / 8;
}

/**
 * @param square
 * @return the rank of the square (0-7)
 */
int file_of(int square) {
    return square % 8;
}

int diagonal_of(int square) {
    return 7 + rank_of(square) - file_of(square);
}

int anti_diagonal_of(int square) {
    return rank_of(square) + file_of(square);
}

int pull_lsb(uint64_t* bb) {
    int square = get_lsb(*bb);
    *bb &= *bb - 1;
    return square;
}

int parse_square(const char *square) {
    int file = square[0] - 'a';
    int rank = square[1] - '0';
    return 8 * (rank - 1) + file;
}

int parse_piece(char piece) {
    switch (piece) {
        case 'P':
            return 0;
        case 'N':
            return 1;
        case 'B':
            return 2;
        case 'R':
            return 3;
        case 'Q':
            return 4;
        case 'K':
            return 5;
        case 'p':
            return 6;
        case 'n':
            return 7;
        case 'b':
            return 8;
        case 'r':
            return 9;
        case 'q':
            return 10;
        case 'k':
            return 11;
        default:
            return 0; // en passant, index neutral
    }
}

void print_move(move_t move) {
    std::cout << (char) ( 'a' + file_of(move.from));
    std::cout << (rank_of(move.from) + 1);
    std::cout << (char) ('a' + file_of(move.to));
    std::cout << (rank_of(move.to) + 1);

    switch (move.flag) {
        case PC_QUEEN:
        case PR_QUEEN:
            std::cout << 'q';
            break;
        case PC_ROOK:
        case PR_ROOK:
            std::cout << 'r';
            break;
        case PC_BISHOP:
        case PR_BISHOP:
            std::cout << 'b';
            break;
        case PC_KNIGHT:
        case PR_KNIGHT:
            std::cout << 'n';
            break;
        default:
            std::cout << ' ';
            break;
    }
}

void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}