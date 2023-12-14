#include <algorithm>
#include <cstring>
#include <vector>

#include "bitboard.h"
#include "util.h"
#include "weights.h"

const move_t move_t::NULL_MOVE = {A1, A1, PASS};
const move_t move_t::CHECKMATE = {A1, A1, PASS};
const move_t move_t::STALEMATE = {H8, H8, PASS};

// Maximum amount of material that can be lost in any exchange
const int32_t MAX_MATERIAL_LOSS = Weights::MATERIAL[piece_t::BLACK_QUEEN];

uint64_t BitUtils::getRandomBitstring()  {
    uint64_t out = 0;
    uint64_t mask = 1ULL;
    int i = 0;
    while (i++ < 63) {
        if (rand() % 2) {
            out |= mask;
        }
        mask <<= 1;
    }
    return out;
}

int32_t move_t::normalizeScore() const {
    return (SCORE_MASK & score) - MAX_MATERIAL_LOSS;
}

void move_t::setScore(move_t::type_t t, int32_t s) {
    score |= (1 << (24 + t));
    s += MAX_MATERIAL_LOSS;
    score |= (s & move_t::SCORE_MASK);
}

bool move_t::isType(move_t::type_t t) const {
    return (score & (1 << (24 + t))) != 0;
}

bool move_t::operator==(const move_t &other) const {
    return to == other.to && from == other.from && flag == other.flag;
}

bool move_t::operator<(const move_t &other) const {
    return score > other.score;
}

std::string move_t::to_string() const {
    std::string out;
    out += ('a' + (char) Bitboard::fileOf(from));
    out += ('1' + (char) Bitboard::rankOf(from));
    out += ('a' + (char) Bitboard::fileOf(to));
    out += ('1' + (char) Bitboard::rankOf(to));

    switch (flag) {
        case PC_QUEEN:
        case PR_QUEEN:
            out += 'q';
            break;
        case PC_ROOK:
        case PR_ROOK:
            out += 'r';
            break;
        case PC_BISHOP:
        case PR_BISHOP:
            out += 'b';
            break;
        case PC_KNIGHT:
        case PR_KNIGHT:
            out += 'n';
            break;
        default:
            break;
    }
    return out;
}

piece_t ConversionUtils::toEnum(char p) {
    switch (p) {
        case 'P':
            return WHITE_PAWN;
        case 'N':
            return WHITE_KNIGHT;
        case 'B':
            return WHITE_BISHOP;
        case 'R':
            return WHITE_ROOK;
        case 'Q':
            return WHITE_QUEEN;
        case 'K':
            return WHITE_KING;
        case 'p':
            return BLACK_PAWN;
        case 'n':
            return BLACK_KNIGHT;
        case 'b':
            return BLACK_BISHOP;
        case 'r':
            return BLACK_ROOK;
        case 'q':
            return BLACK_QUEEN;
        case 'k':
            return BLACK_KING;
        default:
            std::cout << "Bad thing happened" << std::endl;
            return EMPTY;
    }
}

char ConversionUtils::toChar(const piece_t &p) {
    switch (p) {
        case WHITE_PAWN:
            return 'P';
        case WHITE_KNIGHT:
            return 'N';
        case WHITE_BISHOP:
            return 'B';
        case WHITE_ROOK:
            return 'R';
        case WHITE_QUEEN:
            return 'Q';
        case WHITE_KING:
            return 'K';
        case BLACK_PAWN:
            return 'p';
        case BLACK_KNIGHT:
            return 'n';
        case BLACK_BISHOP:
            return 'b';
        case BLACK_ROOK:
            return 'r';
        case BLACK_QUEEN:
            return 'q';
        case BLACK_KING:
            return 'k';
        default:
            return '-';
    }
}

std::string ConversionUtils::moveToString(const move_t &move) {
    std::string out;
    out += ('a' + (char) Bitboard::fileOf(move.from));
    out += ('1' + (char) Bitboard::rankOf(move.from));
    out += ('a' + (char) Bitboard::fileOf(move.to));
    out += ('1' + (char) Bitboard::rankOf(move.to));

    switch (move.flag) {
        case PC_QUEEN:
        case PR_QUEEN:
            out += 'q';
            break;
        case PC_ROOK:
        case PR_ROOK:
            out += 'r';
            break;
        case PC_BISHOP:
        case PR_BISHOP:
            out += 'b';
            break;
        case PC_KNIGHT:
        case PR_KNIGHT:
            out += 'n';
            break;
        default:
            break;
    }
    return out;
}

uint64_t BitUtils::flipBitboardVertical(uint64_t x) {
    const uint64_t k1 = uint64_t(0x00FF00FF00FF00FF);
    const uint64_t k2 = uint64_t(0x0000FFFF0000FFFF);
    x = ((x >> 8) & k1) | ((x & k1) << 8);
    x = ((x >> 16) & k2) | ((x & k2) << 16);
    x = (x >> 32) | (x << 32);
    return x;
}

int BitUtils::getLSB(uint64_t bb) {
    return __builtin_ffsll(bb) - 1;
}

/**
 * Turn on the square on the bitboard.
 * @param bb the bitboard.
 * @param square
 */
void BitUtils::setBit(uint64_t *bb, int square) {
    *bb |= (1ULL << square);
}

/**
 * Clear the square on the bitboard.
 * @param bb the bitboard.
 * @param square
 */
void BitUtils::clearBit(uint64_t *bb, int square) {
    *bb &= ~(1ULL << square);
}

int BitUtils::popCount(uint64_t bb) {
    return __builtin_popcountll(bb);
}

int BitUtils::pullLSB(uint64_t *bb) {
    int square = BitUtils::getLSB(*bb);
    *bb &= *bb - 1;
    return square;
}

bool IOUtils::withinRange(int input, int lower, int upper) {
    return lower < input && input <= upper;
}

int IOUtils::parseSquare(const char *square) {
    int file = square[0] - 'a';
    int rank = square[1] - '0';
    return 8 * (rank - 1) + file;
}

void IOUtils::printMove(move_t move) {
    if (move.from == A1 && move.to == A1 && move.flag == PASS) {
        std::cout << "loss ";
    } else if (move.from == H8 && move.to == H8 && move.flag == PASS) {
        std::cout << "draw ";
    }
    std::cout << (char) ('a' + Bitboard::fileOf(move.from));
    std::cout << (Bitboard::rankOf(move.from) + 1);
    std::cout << (char) ('a' + Bitboard::fileOf(move.to));
    std::cout << (Bitboard::rankOf(move.to) + 1);

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

void IOUtils::printBitstring32(const uint32_t bitstring) {
    uint32_t mask = 1 << 31;
    while (mask) {
        if (mask & bitstring) {
            std::cout << '1';
        } else {
            std::cout << '0';
        }
        mask >>= 1;
    }
    std::cout << '\n';
}

void IOUtils::printBitboard(uint64_t bb) {
    uint64_t iterator = 1;
    char str[64];
    std::memset(str, 0, 64);
    int i = 0;
    while (iterator) {
        str[i++] = (bb & iterator) ? '1' : '0';
        iterator <<= 1;
    }

    for (int r = 7; r >= 0; --r) {
        for (int c = 0; c < 8; ++c) {
            int j = r * 8 + c;
            std::cout << (char) str[j];
        }
        std::cout << '\n';
    }
}

void StringUtils::ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void StringUtils::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void StringUtils::trim(std::string &s) {
    StringUtils::rtrim(s);
    StringUtils::ltrim(s);
}

std::vector<std::string> StringUtils::split(std::string &input) {
    /** Ensures that there exists exactly one trailing space. */
    StringUtils::trim(input);
    input.push_back(' ');

    std::vector<std::string> args;
    std::string tok;
    size_t p;

    while ((p = input.find(' ')) != std::string::npos) {
        tok = input.substr(0, p);
        args.push_back(tok);

        /** Removes newly detached token from string*/
        input.erase(0, p);

        /** Removes spaces between tokens */
        size_t begin = 0;
        while (std::isspace(input[++begin]));
        input.erase(0, begin);
    }
    return args;
}

bool StringUtils::isNumber(const std::string &str) {
    return std::all_of(str.begin(), str.end(), [](char c) { return std::isdigit(c); });
}

bool StringUtils::isNumber(int *ptr, const std::string &arg) {
    if (!StringUtils::isNumber(arg)) {
        return false;
    }
    *ptr = std::stoi(arg);
    return true;
}