#include <cstring>
#include <thread>

#include "bitboard.h"
#include "movegen.h"
#include "search.h"
#include "util.h"
#include "weights.h"

const uint64_t Bitboard::BB_SQUARES[64] = 
{
    1ULL << Squares::A1, 1ULL << Squares::B1, 1ULL << Squares::C1, 1ULL << Squares::D1, 1ULL << Squares::E1, 1ULL << Squares::F1, 1ULL << Squares::G1, 1ULL << Squares::H1,
    1ULL << Squares::A2, 1ULL << Squares::B2, 1ULL << Squares::C2, 1ULL << Squares::D2, 1ULL << Squares::E2, 1ULL << Squares::F2, 1ULL << Squares::G2, 1ULL << Squares::H2,
    1ULL << Squares::A3, 1ULL << Squares::B3, 1ULL << Squares::C3, 1ULL << Squares::D3, 1ULL << Squares::E3, 1ULL << Squares::F3, 1ULL << Squares::G3, 1ULL << Squares::H3,
    1ULL << Squares::A4, 1ULL << Squares::B4, 1ULL << Squares::C4, 1ULL << Squares::D4, 1ULL << Squares::E4, 1ULL << Squares::F4, 1ULL << Squares::G4, 1ULL << Squares::H4,
    1ULL << Squares::A5, 1ULL << Squares::B5, 1ULL << Squares::C5, 1ULL << Squares::D5, 1ULL << Squares::E5, 1ULL << Squares::F5, 1ULL << Squares::G5, 1ULL << Squares::H5,
    1ULL << Squares::A6, 1ULL << Squares::B6, 1ULL << Squares::C6, 1ULL << Squares::D6, 1ULL << Squares::E6, 1ULL << Squares::F6, 1ULL << Squares::G6, 1ULL << Squares::H6,
    1ULL << Squares::A7, 1ULL << Squares::B7, 1ULL << Squares::C7, 1ULL << Squares::D7, 1ULL << Squares::E7, 1ULL << Squares::F7, 1ULL << Squares::G7, 1ULL << Squares::H7,
    1ULL << Squares::A8, 1ULL << Squares::B8, 1ULL << Squares::C8, 1ULL << Squares::D8, 1ULL << Squares::E8, 1ULL << Squares::F8, 1ULL << Squares::G8, 1ULL << Squares::H8
};

const uint64_t Bitboard::BB_ALL = 0xffffffffffffffff;

const uint64_t Bitboard::BB_FILE_A = 0x0101010101010101;
const uint64_t Bitboard::BB_FILE_B = Bitboard::BB_FILE_A << 1;
const uint64_t Bitboard::BB_FILE_C = Bitboard::BB_FILE_A << 2;
const uint64_t Bitboard::BB_FILE_D = Bitboard::BB_FILE_A << 3;
const uint64_t Bitboard::BB_FILE_E = Bitboard::BB_FILE_A << 4;
const uint64_t Bitboard::BB_FILE_F = Bitboard::BB_FILE_A << 5;
const uint64_t Bitboard::BB_FILE_G = Bitboard::BB_FILE_A << 6;
const uint64_t Bitboard::BB_FILE_H = Bitboard::BB_FILE_A << 7;
const uint64_t Bitboard::BB_FILES[8] = 
{
    Bitboard::BB_FILE_A, 
    Bitboard::BB_FILE_B, 
    Bitboard::BB_FILE_C, 
    Bitboard::BB_FILE_D, 
    Bitboard::BB_FILE_E, 
    Bitboard::BB_FILE_F, 
    Bitboard::BB_FILE_G, 
    Bitboard::BB_FILE_H
};

const uint64_t Bitboard::BB_RANK_1 = 0xff;
const uint64_t Bitboard::BB_RANK_2 = Bitboard::BB_RANK_1 << 8;
const uint64_t Bitboard::BB_RANK_3 = Bitboard::BB_RANK_1 << 16;
const uint64_t Bitboard::BB_RANK_4 = Bitboard::BB_RANK_1 << 24;
const uint64_t Bitboard::BB_RANK_5 = Bitboard::BB_RANK_1 << 32;
const uint64_t Bitboard::BB_RANK_6 = Bitboard::BB_RANK_1 << 40;
const uint64_t Bitboard::BB_RANK_7 = Bitboard::BB_RANK_1 << 48;
const uint64_t Bitboard::BB_RANK_8 = Bitboard::BB_RANK_1 << 56;
const uint64_t Bitboard::BB_RANKS[8] = 
{
    Bitboard::BB_RANK_1, 
    Bitboard::BB_RANK_2, 
    Bitboard::BB_RANK_3, 
    Bitboard::BB_RANK_4, 
    Bitboard::BB_RANK_5, 
    Bitboard::BB_RANK_6, 
    Bitboard::BB_RANK_7, 
    Bitboard::BB_RANK_8
};


const uint64_t Bitboard::BB_DIAGONAL_1 = 0x80;
const uint64_t Bitboard::BB_DIAGONAL_2 = 0x8040;
const uint64_t Bitboard::BB_DIAGONAL_3 = 0x804020;
const uint64_t Bitboard::BB_DIAGONAL_4 = 0x80402010;
const uint64_t Bitboard::BB_DIAGONAL_5 = 0x8040201008;
const uint64_t Bitboard::BB_DIAGONAL_6 = 0x804020100804;
const uint64_t Bitboard::BB_DIAGONAL_7 = 0x80402010080402;
const uint64_t Bitboard::BB_DIAGONAL_8 = 0x8040201008040201;
const uint64_t Bitboard::BB_DIAGONAL_9 = 0x4020100804020100;
const uint64_t Bitboard::BB_DIAGONAL_10 = 0x2010080402010000;
const uint64_t Bitboard::BB_DIAGONAL_11 = 0x1008040201000000;
const uint64_t Bitboard::BB_DIAGONAL_12 = 0x804020100000000;
const uint64_t Bitboard::BB_DIAGONAL_13 = 0x402010000000000;
const uint64_t Bitboard::BB_DIAGONAL_14 = 0x201000000000000;
const uint64_t Bitboard::BB_DIAGONAL_15 = 0x100000000000000;

const uint64_t Bitboard::BB_DIAGONALS[15] =
{
    Bitboard::BB_DIAGONAL_1, 
    Bitboard::BB_DIAGONAL_2, 
    Bitboard::BB_DIAGONAL_3, 
    Bitboard::BB_DIAGONAL_4, 
    Bitboard::BB_DIAGONAL_5,
    Bitboard::BB_DIAGONAL_6,
    Bitboard::BB_DIAGONAL_7, 
    Bitboard::BB_DIAGONAL_8, 
    Bitboard::BB_DIAGONAL_9, 
    Bitboard::BB_DIAGONAL_10, 
    Bitboard::BB_DIAGONAL_11,
    Bitboard::BB_DIAGONAL_12,
    Bitboard::BB_DIAGONAL_13, 
    Bitboard::BB_DIAGONAL_14, 
    Bitboard::BB_DIAGONAL_15
};


const uint64_t Bitboard::BB_ANTI_DIAGONAL_1 = 0x1;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_2 = 0x102;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_3 = 0x10204;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_4 = 0x1020408;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_5 = 0x102040810;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_6 = 0x10204081020;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_7 = 0x1020408102040;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_8 = 0x102040810204080;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_9 = 0x204081020408000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_10 = 0x408102040800000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_11 = 0x810204080000000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_12 = 0x1020408000000000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_13 = 0x2040800000000000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_14 = 0x4080000000000000;
const uint64_t Bitboard::BB_ANTI_DIAGONAL_15 = 0x8000000000000000;

const uint64_t Bitboard::BB_ANTI_DIAGONALS[15] = 
{
    Bitboard::BB_ANTI_DIAGONAL_1, 
    Bitboard::BB_ANTI_DIAGONAL_2, 
    Bitboard::BB_ANTI_DIAGONAL_3, 
    Bitboard::BB_ANTI_DIAGONAL_4,
    Bitboard::BB_ANTI_DIAGONAL_5, 
    Bitboard::BB_ANTI_DIAGONAL_6, 
    Bitboard::BB_ANTI_DIAGONAL_7, 
    Bitboard::BB_ANTI_DIAGONAL_8, 
    Bitboard::BB_ANTI_DIAGONAL_9, 
    Bitboard::BB_ANTI_DIAGONAL_10, 
    Bitboard::BB_ANTI_DIAGONAL_11, 
    Bitboard::BB_ANTI_DIAGONAL_12,
    Bitboard::BB_ANTI_DIAGONAL_13, 
    Bitboard::BB_ANTI_DIAGONAL_14, 
    Bitboard::BB_ANTI_DIAGONAL_15
};

uint64_t Bitboard::BB_RAYS[64][64];

const int Bitboard::MAX_MOVE_NUM = 218;
const int Bitboard::MAX_CAPTURE_NUM = 74;
const int Bitboard::MAX_ATTACK_NUM = 16;

uint64_t Bitboard::ZOBRIST_VALUES[781];

int Bitboard::fileOf(int square) {
    return square % 8;
}

int Bitboard::rankOf(int square) {
    return square / 8;
}

int Bitboard::diagonalOf(int square) {
    return 7 + Bitboard::rankOf(square) - Bitboard::fileOf(square);
}

int Bitboard::antiDiagonalOf(int square) {
    return Bitboard::rankOf(square) + Bitboard::fileOf(square);
}

uint64_t Bitboard::getRayBetween(int square1, int square2) {
    return (Bitboard::BB_RAYS[square1][square2] & ((Bitboard::BB_ALL << square1) ^ (Bitboard::BB_ALL << square2))) | Bitboard::BB_SQUARES[square2];
}

uint64_t Bitboard::getRayBetweenInclusive(int square1, int square2) {
    return Bitboard::BB_RAYS[square1][square2];
}

void Bitboard::initializeZobrist() {
    for (int i = 0; i < 781; ++i) Bitboard::ZOBRIST_VALUES[i] = BitUtils::getRandomBitstring();
}

Bitboard::Bitboard(const std::string &fen) {
    char *rest = strdup(fen.c_str());
    char *og_rest = rest;
    // Initalize bitboards and mailbox
    char *token = strtok_r(rest, " ", &rest);
    for (int i = Squares::A1; i <= Squares::H8; ++i) {
        this->mailbox[i] = piece_t::EMPTY;
    }
    this->wPawns = 0;
    this->wKnights = 0;
    this->wBishops = 0;
    this->wRooks = 0;
    this->wQueens = 0;
    this->wKing = 0;
    this->bPawns = 0;
    this->bKnights = 0;
    this->bBishops = 0;
    this->bRooks = 0;
    this->bQueens = 0;
    this->bKing = 0;
    for (int rank = 7; rank >= 0; --rank) {
        char *fen_board = strtok_r(token, "/", &token);
        int file = 0;
        const std::size_t len = strlen(fen_board);
        for (int j = 0; j < len; ++j) {
            if (file >= 8) break;

            char piece = fen_board[j];
            if (isdigit(piece)) {
                file += piece - '0';
            } else {
                int square = 8 * rank + file;
                this->mailbox[square] = ConversionUtils::toEnum(piece);
                uint64_t *bitboard = this->getBitboard(ConversionUtils::toEnum(piece));
                BitUtils::setBit(bitboard, square);
                ++file;
            }
        }
    }
    this->wOccupied =
            this->wPawns | this->wKnights | this->wBishops | this->wRooks | this->wQueens | this->wKing;
    this->bOccupied =
            this->bPawns | this->bKnights | this->bBishops | this->bRooks | this->bQueens | this->bKing;
    this->occupied = this->wOccupied | this->bOccupied;

    // Initalize king squares
    this->wKingSquare = BitUtils::getLSB(this->wKing);
    this->bKingSquare = BitUtils::getLSB(this->bKing);

    // Initalize turn
    token = strtok_r(rest, " ", &rest);
    this->turn = (*token == 'w') ? WHITE : BLACK;

    // Initalize castling rights
    token = strtok_r(rest, " ", &rest);
    this->wKingsideCastleRights = false;
    this->wQueensideCastleRights = false;
    this->bKingsideCastleRights = false;
    this->bQueensideCastleRights = false;
    for (int i = 0, j = strlen(token); i < j; i++) {
        char piece = token[i];
        switch (piece) {
            case 'K':
                this->wKingsideCastleRights = true;
                break;
            case 'Q':
                this->wQueensideCastleRights = true;
                break;
            case 'k':
                this->bKingsideCastleRights = true;
                break;
            case 'q':
                this->bQueensideCastleRights = true;
                break;
        }
    }
    // Initalize possible en passant square
    token = strtok_r(rest, " ", &rest);
    this->en_passant_square = (*token == '-') ? INVALID : IOUtils::parseSquare(token);
    // Initalize halfmove clock
    token = strtok_r(rest, " ", &rest);

    if (token) this->halfmove_clock = std::strtol(token, nullptr, 10);
    else goto HASH;
    // Initalize fullmove number
    token = strtok_r(rest, " ", &rest);
    if (token) this->fullmove_number = std::strtol(token, nullptr, 10);
    
    HASH:
    this->hash_code = 0;
    for (int square = Squares::A1; square <= Squares::H8; square++) {
        piece_t piece = this->mailbox[square];
        if (piece != EMPTY) {
            this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * (int) piece + square];
        }
    }
    if (this->turn == BLACK) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[768];
    }
    if (this->wKingsideCastleRights) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[769];
    }
    if (this->wQueensideCastleRights) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[770];
    }
    if (this->bKingsideCastleRights) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[771];
    }
    if (this->bQueensideCastleRights) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[772];
    }
    if (this->en_passant_square != INVALID) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[773 + Bitboard::fileOf(this->en_passant_square)];
    }
    free(og_rest);
}

Bitboard::Bitboard() {}

int Bitboard::genLegalMoves(move_t *moves, bool color) {
    int i = 0;
    uint64_t pieces;
    uint64_t king_bb;
    int king_square;
    uint64_t enemy_pawns_attacks;
    if (color == WHITE) {
        pieces = this->wOccupied;
        king_bb = this->wKing;
        king_square = this->wKingSquare;
        enemy_pawns_attacks = (((this->bPawns >> 9) & ~Bitboard::BB_FILE_H) | ((this->bPawns >> 7) & ~Bitboard::BB_FILE_A)) & this->wOccupied;
    } else {
        pieces = this->bOccupied;
        king_bb = this->bKing;
        king_square = this->bKingSquare;
        enemy_pawns_attacks = (((this->wPawns << 9) & ~Bitboard::BB_FILE_A) | ((this->wPawns << 7) & ~Bitboard::BB_FILE_H)) & this->bOccupied;
    }

    uint64_t attackmask = this->_get_attackmask(!color);
    uint64_t checkmask = this->_get_checkmask(color);
    uint64_t pos_pinned = this->get_queen_moves(!color, king_square) & pieces;

    // King is in double check, only moves are to move king away
    if (!checkmask) {
        uint64_t moves_bb = this->get_king_moves(color, king_square) & ~attackmask;
        while (moves_bb) {
            int to = BitUtils::pullLSB(&moves_bb);
            MoveFlags flag = this->getFlag(piece_t::BLACK_KING, king_square, to);
            if (flag == MoveFlags::CASTLING) continue;
            move_t move = {(unsigned int) king_square, (unsigned int) to, (unsigned int) flag};
            moves[i++] = move;
        }
        return i;
    }

    while (pieces) {
        const int from = BitUtils::pullLSB(&pieces);
        piece_t piece = static_cast<piece_t>(this->mailbox[from] % 6);

        uint64_t pinned_bb = Bitboard::BB_SQUARES[from] & pos_pinned;
        uint64_t pinmask = pinned_bb ? this->_get_pinmask(color, from) : Bitboard::BB_ALL;

        uint64_t moves_bb;
        switch (piece) {
            case piece_t::BLACK_PAWN: {
                uint64_t pawn_moves = this->get_pawn_moves(color, from);
                moves_bb = pawn_moves & checkmask & pinmask;

                if (this->en_passant_square != INVALID) {
                    if (pawn_moves & pinmask & Bitboard::BB_SQUARES[this->en_passant_square]) {
                        // Add possible en passant capture to remove check
                        // For example en passant is legal here:
                        // 8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1
                        if (king_bb & enemy_pawns_attacks) {
                            BitUtils::setBit(&moves_bb, this->en_passant_square);
                        }
                    }
                }

                break;
            }
            case piece_t::BLACK_KNIGHT:
                moves_bb = this->get_knight_moves(color, from) & checkmask & pinmask;
                break;
            case piece_t::BLACK_BISHOP:
                moves_bb = this->get_bishop_moves(color, from) & checkmask & pinmask;
                break;
            case piece_t::BLACK_ROOK:
                moves_bb = this->get_rook_moves(color, from) & checkmask & pinmask;
                break;
            case piece_t::BLACK_QUEEN:
                moves_bb = this->get_queen_moves(color, from) & checkmask & pinmask;
                break;
            case piece_t::BLACK_KING:
                moves_bb = this->get_king_moves(color, from) & ~attackmask;
                break;
            default:
                exit(-1);
        }

        while (moves_bb) {
            int to = BitUtils::pullLSB(&moves_bb);
            if (piece == piece_t::BLACK_PAWN && (Bitboard::rankOf(to) == 0 || Bitboard::rankOf(to) == 7)) { // Add all promotions
                if (this->mailbox[to] == piece_t::EMPTY) {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PR_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PR_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PR_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PR_KNIGHT};
                    moves[i++] = knight_promotion;
                } else {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_KNIGHT};
                    moves[i++] = knight_promotion;
                }

            } else {
                int flag = this->getFlag(piece, from, to);
                move_t move = {(unsigned int) from, (unsigned int) to, (unsigned int) flag};

                // Determine if castling is legal
                if (flag == MoveFlags::CASTLING) {
                    if (attackmask & king_bb) continue; // Assert the king is not in check
                    if (color == WHITE) {
                        if (from != Squares::E1) continue; // Assert the king is still alive
                        if (to == Squares::G1) { // Kingside
                            if (!this->wKingsideCastleRights) continue; // Assert king or rook has not moved
                            if (!(this->wRooks & Bitboard::BB_SQUARES[Squares::H1])) continue; // Assert rook is still alive
                            if (this->occupied & (Bitboard::BB_SQUARES[Squares::F1] | Bitboard::BB_SQUARES[Squares::G1]))
                                continue; // Assert there are no pieces between the king and rook
                            if (attackmask & (Bitboard::BB_SQUARES[F1] | Bitboard::BB_SQUARES[Squares::G1]))
                                continue; // Assert the squares the king moves through are not attacked
                        } else if (to == Squares::C1) { // Queenside
                            if (!this->wQueensideCastleRights) continue;
                            if (!(this->wRooks & Bitboard::BB_SQUARES[Squares::A1])) continue;
                            if (this->occupied & (Bitboard::BB_SQUARES[Squares::D1] | Bitboard::BB_SQUARES[Squares::C1] | Bitboard::BB_SQUARES[Squares::B1])) continue;
                            if (attackmask & (Bitboard::BB_SQUARES[Squares::D1] | Bitboard::BB_SQUARES[Squares::C1])) continue;
                        } else {
                            continue;
                        }
                    } else {
                        if (from != Squares::E8) continue;
                        if (to == Squares::G8) { // Kingside
                            if (!this->bKingsideCastleRights) continue;
                            if (!(this->bRooks & Bitboard::BB_SQUARES[Squares::H8])) continue;
                            if (this->occupied & (Bitboard::BB_SQUARES[Squares::F8] | Bitboard::BB_SQUARES[Squares::G8])) continue;
                            if (attackmask & (Bitboard::BB_SQUARES[Squares::F8] | Bitboard::BB_SQUARES[Squares::G8])) continue;
                        } else if (to == Squares::C8) { // Queenside
                            if (!this->bQueensideCastleRights) continue;
                            if (!(this->bRooks & Bitboard::BB_SQUARES[Squares::A8])) continue;
                            if (this->occupied & (Bitboard::BB_SQUARES[Squares::D8] | Bitboard::BB_SQUARES[Squares::C8] | Bitboard::BB_SQUARES[Squares::B8])) continue;
                            if (attackmask & (Bitboard::BB_SQUARES[Squares::D8] | Bitboard::BB_SQUARES[Squares::C8])) continue;
                        } else {
                            continue;
                        }
                    }
                } else if (flag == MoveFlags::EN_PASSANT) {
                    // Remove possible en passant capture that leaves king in check
                    // For example en passant is illegal here:
                    // 8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1
                    // k7/1q6/8/3pP3/8/5K2/8/8 w - d6 0 1
                    const Bitboard copy = *this;
                    this->makeMove(move);
                    bool invalid = this->isInCheck(color);
                    *this = copy;
                    if (invalid) continue;
                }
                moves[i++] = move;
            }
        }
    }
    return i;
}

int Bitboard::genLegalCaptures(move_t *moves, bool color) {
    int i = 0;

    uint64_t pieces;
    uint64_t kingBb;
    int kingSquare;
    uint64_t enemyPawnAttacks;
    uint64_t enemyBb;
    if (color == WHITE) {
        pieces = this->wOccupied;
        kingBb = this->wKing;
        kingSquare = this->wKingSquare;
        enemyPawnAttacks = (((this->bPawns >> 9) & ~Bitboard::BB_FILE_H) | ((this->bPawns >> 7) & ~Bitboard::BB_FILE_A))
                              & this->wOccupied;
        enemyBb = this->bOccupied;
    } else {
        pieces = this->bOccupied;
        kingBb = this->bKing;
        kingSquare = this->bKingSquare;
        enemyPawnAttacks = (((this->wPawns << 9) & ~Bitboard::BB_FILE_A) | ((this->wPawns << 7) & ~Bitboard::BB_FILE_H))
                              & this->bOccupied;
        enemyBb = this->wOccupied;
    }

    uint64_t attackmask = this->_get_attackmask(!color);
    uint64_t checkmask = this->_get_checkmask(color);
    uint64_t posPinned = this->get_queen_moves(!color, kingSquare) & pieces;

    // King is in double check, only moves are to king moves away that are captures
    if (!checkmask) {
        uint64_t moves_bb = this->get_king_moves(color, kingSquare) & ~attackmask & enemyBb;
        while (moves_bb) {
            int to = BitUtils::pullLSB(&moves_bb);
            int flag = this->getFlag(piece_t::BLACK_KING, kingSquare, to);
            move_t move = {(unsigned int) kingSquare, (unsigned int) to, (unsigned int) flag};
            moves[i++] = move;
        }
        return i;
    }

    while (pieces) {
        const int from = BitUtils::pullLSB(&pieces);
        piece_t piece = static_cast<piece_t> (this->mailbox[from] % 6);
        uint64_t pinnedBb = Bitboard::BB_SQUARES[from] & posPinned;
        uint64_t pinmask = pinnedBb ? this->_get_pinmask(color, from) : Bitboard::BB_ALL;

        uint64_t movesBb;
        switch (piece) {
            case piece_t::BLACK_PAWN: {
                uint64_t pawnMoves = this->get_pawn_moves(color, from);
                movesBb = pawnMoves & checkmask & pinmask & enemyBb;

                if (this->en_passant_square != INVALID) {
                    if (pawnMoves & pinmask & Bitboard::BB_SQUARES[this->en_passant_square]) {
                        // Add possible en passant capture to remove check
                        // For example en passant is legal here:
                        // 8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1
                        if (kingBb & enemyPawnAttacks) {
                            BitUtils::setBit(&movesBb, this->en_passant_square);
                        }
                    }
                }
                break;
            }
            case piece_t::BLACK_KNIGHT:
                movesBb = this->get_knight_moves(color, from) & checkmask & pinmask & enemyBb;
                break;
            case piece_t::BLACK_BISHOP:
                movesBb = this->get_bishop_moves(color, from) & checkmask & pinmask & enemyBb;
                break;
            case piece_t::BLACK_ROOK:
                movesBb = this->get_rook_moves(color, from) & checkmask & pinmask & enemyBb;
                break;
            case piece_t::BLACK_QUEEN:
                movesBb = this->get_queen_moves(color, from) & checkmask & pinmask & enemyBb;
                break;
            case piece_t::BLACK_KING:
                movesBb = get_king_moves_no_castle(color, from) & ~attackmask & enemyBb;
                break;
            default:
                std::cout << "Error on movegen.cpp line 509 in gen_legal_captures(move_t *, bool)\n";
                exit(-1);
        }

        while (movesBb) {
            int to = BitUtils::pullLSB(&movesBb);
            if (piece == piece_t::BLACK_PAWN && (Bitboard::rankOf(to) == 0 || Bitboard::rankOf(to) == 7)) { // Add all promotion captures
                if (this->mailbox[to] != piece_t::EMPTY) {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, MoveFlags::PC_KNIGHT};
                    moves[i++] = knight_promotion;
                }
            } else {
                MoveFlags flag = this->getFlag(piece, from, to);
                move_t move = {(unsigned int) from, (unsigned int) to, (unsigned int) flag};

                if (flag == MoveFlags::EN_PASSANT) {
                    // Remove possible en passant capture that leaves king in check
                    // For example en passant is illegal here:
                    // 8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1
                    // k7/1q6/8/3pP3/8/5K2/8/8 w - d6 0 1
                    const Bitboard copy = *this;
                    this->makeMove(move);
                    bool invalid = this->isInCheck(color);
                    *this = copy;
                    if (invalid) continue;
                }
                moves[i++] = move;
            }
        }
    }
    return i;
}

int Bitboard::genNonquiescentMoves(move_t *moves, bool color) {
    int num_proms = 0;

    uint64_t pawns, pieces;
    int king_square;
    if (color) {
        pieces = this->wOccupied;
        king_square = this->wKingSquare;

        uint64_t checkmask = this->_get_checkmask(color);
        uint64_t pos_pinned = this->get_queen_moves(BLACK, king_square) & pieces;

        pawns = this->wPawns & Bitboard::BB_RANK_7;
        while (pawns) {
            int from = BitUtils::pullLSB(&pawns);

            uint64_t pinmask;
            uint64_t pinned_bb = Bitboard::BB_SQUARES[from] & pos_pinned;
            if (pinned_bb) {
                pinmask = _get_pinmask(color, from);
            } else {
                pinmask = Bitboard::BB_ALL;
            }
            if (!(((Bitboard::BB_SQUARES[from] << 8) & ~(this->occupied)) & checkmask & pinmask)) continue;
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from + 8, MoveFlags::PR_QUEEN, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from + 8, MoveFlags::PR_ROOK, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from + 8, MoveFlags::PR_BISHOP, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from + 8, MoveFlags::PR_KNIGHT, 0};
        }
    } else {
        pieces = this->bOccupied;
        king_square = this->bKingSquare;

        uint64_t checkmask = this->_get_checkmask(color);
        uint64_t pos_pinned = this->get_queen_moves(WHITE, king_square) & pieces;

        pawns = this->bPawns & Bitboard::BB_RANK_2;
        while (pawns) {
            int from = BitUtils::pullLSB(&pawns);

            uint64_t pinned_bb = Bitboard::BB_SQUARES[from] & pos_pinned;
            uint64_t pinmask = pinned_bb ? this->_get_pinmask(color, from) : Bitboard::BB_ALL;

            if (!(((Bitboard::BB_SQUARES[from] >> 8) & ~(this->occupied)) & checkmask & pinmask)) continue;
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from - 8, MoveFlags::PR_QUEEN, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from - 8, MoveFlags::PR_ROOK, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from - 8, MoveFlags::PR_BISHOP, 0};
            moves[num_proms++] = {(unsigned int) from, (unsigned int) from - 8, MoveFlags::PR_KNIGHT, 0};
        }
    }
    int num_captures = this->genLegalCaptures(&(moves[num_proms]), color);
    return num_proms + num_captures;
}

/**
 * Updates the board with the move.
 * @param move
 */
void Bitboard::makeMove(const move_t &move) {
    int from = move.from;
    int to = move.to;
    int flag = move.flag;
    bool color = this->turn;

    piece_t attacker = this->mailbox[from];
    piece_t victim = this->mailbox[to];
    if (flag == PASS) {
        this->turn = !color;
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[768];
        return;
    }

    bool reset_halfmove = false;
    if (this->en_passant_square != INVALID) {
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[773 + Bitboard::fileOf(this->en_passant_square)];
        this->en_passant_square = INVALID;
    }
    uint64_t *attacker_bb = this->getBitboard(attacker);
    BitUtils::clearBit(attacker_bb, from);
    BitUtils::setBit(attacker_bb, to);
    this->mailbox[from] = piece_t::EMPTY;
    this->mailbox[to] = attacker;
    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * (int) attacker + from];
    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * (int) attacker + to];

    switch (attacker) {
        case piece_t::WHITE_PAWN:
            reset_halfmove = true;
            if (Bitboard::rankOf(to) - Bitboard::rankOf(from) == 2) {
                this->en_passant_square = to - 8;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[773 + Bitboard::fileOf(this->en_passant_square)];
            } else if (flag == MoveFlags::EN_PASSANT) {
                BitUtils::clearBit(&this->bPawns, to - 8);
                this->mailbox[to - 8] = EMPTY;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * BLACK_PAWN + (to - 8)];
            } else if (Bitboard::rankOf(to) == 7) { // Promotions
                BitUtils::clearBit(&this->wPawns, to);
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * WHITE_PAWN + to];
                switch (flag) {
                    case PR_QUEEN:
                        BitUtils::setBit(&this->wQueens, to);
                        this->mailbox[to] = WHITE_QUEEN;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * WHITE_QUEEN + to];
                        break;
                    case PR_ROOK:
                        BitUtils::setBit(&this->wRooks, to);
                        this->mailbox[to] = WHITE_ROOK;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * WHITE_ROOK + to];
                        break;
                    case PR_BISHOP:
                        BitUtils::setBit(&this->wBishops, to);
                        this->mailbox[to] = WHITE_BISHOP;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * WHITE_BISHOP + to];
                        break;
                    case PR_KNIGHT:
                        BitUtils::setBit(&this->wKnights, to);
                        this->mailbox[to] = WHITE_KNIGHT;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * WHITE_KNIGHT + to];
                        break;
                }
            }
            break;
        case piece_t::WHITE_ROOK:
            if (from == Squares::H1 && this->wKingsideCastleRights) {
                this->wKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[769];
            } else if (from == Squares::A1 && this->wQueensideCastleRights) {
                this->wQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[770];
            }
            break;
        case piece_t::WHITE_KING:
            this->wKingSquare = to;
            if (flag == MoveFlags::CASTLING) {
                if (Bitboard::fileOf(to) - Bitboard::fileOf(from) > 0) { // Kingside
                    BitUtils::clearBit(&this->wRooks, Squares::H1);
                    BitUtils::setBit(&this->wRooks, Squares::F1);
                    this->mailbox[H1] = piece_t::EMPTY;
                    this->mailbox[F1] = piece_t::WHITE_ROOK;
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::WHITE_ROOK + Squares::H1];
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::WHITE_ROOK + Squares::F1];
                } else { // Queenside
                    BitUtils::clearBit(&this->wRooks, Squares::A1);
                    BitUtils::setBit(&this->wRooks, Squares::D1);
                    this->mailbox[Squares::A1] = piece_t::EMPTY;
                    this->mailbox[Squares::D1] = piece_t::WHITE_ROOK;
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::WHITE_ROOK + Squares::A1];
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::WHITE_ROOK + Squares::D1];
                }
            }

            if (this->wKingsideCastleRights) {
                this->wKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[769];
            }
            if (this->wQueensideCastleRights) {
                this->wQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[770];
            }
            break;
        case piece_t::BLACK_PAWN:
            reset_halfmove = true;
            if (Bitboard::rankOf(to) - Bitboard::rankOf(from) == -2) {
                this->en_passant_square = to + 8;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[773 + Bitboard::fileOf(this->en_passant_square)];
            } else if (flag == MoveFlags::EN_PASSANT) {
                BitUtils::clearBit(&this->wPawns, to + 8);
                this->mailbox[to + 8] = piece_t::EMPTY;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::WHITE_PAWN + (to + 8)];
            } else if (Bitboard::rankOf(to) == 0) { // Promotions
                BitUtils::clearBit(&this->bPawns, to);
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_PAWN + to];
                switch (flag) {
                    case MoveFlags::PR_QUEEN:
                        BitUtils::setBit(&this->bQueens, to);
                        this->mailbox[to] = piece_t::BLACK_QUEEN;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_QUEEN + to];
                        break;
                    case MoveFlags::PR_ROOK:
                        BitUtils::setBit(&this->bRooks, to);
                        this->mailbox[to] = piece_t::BLACK_ROOK;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_ROOK + to];
                        break;
                    case MoveFlags::PR_BISHOP:
                        BitUtils::setBit(&this->bBishops, to);
                        this->mailbox[to] = piece_t::BLACK_BISHOP;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_BISHOP + to];
                        break;
                    case MoveFlags::PR_KNIGHT:
                        BitUtils::setBit(&this->bKnights, to);
                        this->mailbox[to] = piece_t::BLACK_KNIGHT;
                        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_KNIGHT + to];
                        break;
                }
            }
            break;
        case piece_t::BLACK_ROOK:
            if (from == Squares::H8 && this->bKingsideCastleRights) {
                this->bKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[771];
            } else if (from == Squares::A8 && this->bQueensideCastleRights) {
                this->bQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[772];
            }
            break;
        case piece_t::BLACK_KING:
            this->bKingSquare = to;
            if (flag == MoveFlags::CASTLING) {
                if (Bitboard::fileOf(to) - Bitboard::fileOf(from) > 0) { // Kingside
                    BitUtils::clearBit(&this->bRooks, Squares::H8);
                    BitUtils::setBit(&this->bRooks, Squares::F8);
                    this->mailbox[Squares::H8] = piece_t::EMPTY;
                    this->mailbox[Squares::F8] = piece_t::BLACK_ROOK;
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_ROOK + Squares::H8];
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_ROOK + Squares::F8];
                } else { // Queenside
                    BitUtils::clearBit(&this->bRooks, Squares::A8);
                    BitUtils::setBit(&this->bRooks, Squares::D8);
                    this->mailbox[Squares::A8] = piece_t::EMPTY;
                    this->mailbox[Squares::D8] = piece_t::BLACK_ROOK;
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_ROOK + Squares::A8];
                    this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * piece_t::BLACK_ROOK + Squares::D8];
                }
            }

            if (this->bKingsideCastleRights) {
                this->bKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[771];
            }
            if (this->bQueensideCastleRights) {
                this->bQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[772];
            }
            break;
    }
    if (victim != piece_t::EMPTY) {
        reset_halfmove = true;
        uint64_t *victim_bb = this->getBitboard(victim);
        BitUtils::clearBit(victim_bb, to);
        this->hash_code ^= Bitboard::ZOBRIST_VALUES[64 * (int) victim + to];
        if (this->wKingsideCastleRights) {
            if (to == Squares::H1) {
                this->wKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[769];
            }
        } else if (this->wQueensideCastleRights) {
            if (to == Squares::A1) {
                this->wQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[770];
            }
        } else if (this->bQueensideCastleRights) {
            if (to == Squares::A8) {
                this->bQueensideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[771];
            }
        } else if (this->bKingsideCastleRights) {
            if (to == Squares::H8) {
                this->bKingsideCastleRights = false;
                this->hash_code ^= Bitboard::ZOBRIST_VALUES[772];
            }
        }
    }
    this->wOccupied =
            this->wPawns | this->wKnights | this->wBishops | this->wRooks | this->wQueens | this->wKing;
    this->bOccupied =
            this->bPawns | this->bKnights | this->bBishops | this->bRooks | this->bQueens | this->bKing;
    this->occupied = this->wOccupied | this->bOccupied;
    if (reset_halfmove) {
        this->halfmove_clock = 0;
    } else {
        this->halfmove_clock++;
    }
    this->turn = !color;
    this->fullmove_number += color;
    this->hash_code ^= Bitboard::ZOBRIST_VALUES[768];
}

move_t Bitboard::parseMove(const std::string &mv_str) {
    if (mv_str.size() < 4) {
        return move_t::NULL_MOVE;
    }
    move_t moves[MAX_MOVE_NUM];
    int n = this->genLegalMoves(moves, this->turn);
    for (int i = 0; i < n; ++i) {
        int src_file = int(mv_str[0] - 'a');
        int src_rank = int(mv_str[1] - '1');
        int dest_file = int(mv_str[2] - 'a');
        int dest_rank = int(mv_str[3] - '1');

        if ((moves[i].flag > MoveFlags::CAPTURE) && (mv_str.size() < 5)) {
            // Some sort of promotion is happening. However, the move string does not contain a 5th character.
            continue;
        }

        if (moves[i].from == 8 * src_rank + src_file && moves[i].to == 8 * dest_rank + dest_file) {
            if (moves[i].flag == MoveFlags::NONE) {
                return moves[i];
            } else if ((moves[i].flag == MoveFlags::PC_QUEEN || moves[i].flag == MoveFlags::PR_QUEEN) && mv_str[4] == 'q') {
                return moves[i];
            } else if ((moves[i].flag == MoveFlags::PC_ROOK || moves[i].flag == MoveFlags::PR_ROOK) && mv_str[4] == 'r') {
                return moves[i];
            } else if ((moves[i].flag == MoveFlags::PC_BISHOP || moves[i].flag == MoveFlags::PR_BISHOP) && mv_str[4] == 'b') {
                return moves[i];
            } else if ((moves[i].flag == MoveFlags::PC_KNIGHT || moves[i].flag == MoveFlags::PR_KNIGHT) && mv_str[4] == 'n') {
                return moves[i];
            }
        }
    }
    return move_t::NULL_MOVE;
}

bool Bitboard::isInCheck(bool color) {
    if (color == WHITE) {
        return this->isAttacked(BLACK, BitUtils::getLSB(this->wKing));
    } else {
        return this->isAttacked(WHITE, BitUtils::getLSB(this->bKing));
    }
}

bool Bitboard::isMoveCheck(const move_t &move) {
    Bitboard savedBoard = *this;
    this->makeMove(move);
    bool inCheck = this->isInCheck(this->turn);
    *this = savedBoard;
    return inCheck;
}

/**
 * @param color the color of the attackers.
 * @param square the square potentially being attacked.
 * @return true if the square is being attacked by the given side
 */
bool Bitboard::isAttacked(bool color, int square) {
    if (color == BLACK) {
        uint64_t square_bb = Bitboard::BB_SQUARES[square];
        if (this->get_queen_moves(WHITE, square) & this->bQueens) return true;
        if (this->get_rook_moves(WHITE, square) & this->bRooks) return true;
        if (this->get_bishop_moves(WHITE, square) & this->bBishops) return true;
        if (this->get_knight_moves(WHITE, square) & this->bKnights) return true;
        if ((((square_bb << 9) & ~BB_FILE_A) | ((square_bb << 7) & ~BB_FILE_H)) & this->bPawns) return true;
        return false;
    } else {
        uint64_t square_bb = Bitboard::BB_SQUARES[square];
        if (this->get_queen_moves(BLACK, square) & this->wQueens) return true;
        if (this->get_rook_moves(BLACK, square) & this->wRooks) return true;
        if (this->get_bishop_moves(BLACK, square) & this->wBishops) return true;
        if (this->get_knight_moves(BLACK, square) & this->wKnights) return true;
        if ((((square_bb >> 9) & ~BB_FILE_H) | ((square_bb >> 7) & ~BB_FILE_A)) & this->wPawns) return true;
        return false;
    }
}

bool Bitboard::containsPromotions() {
    uint64_t promotionSquares;
    if (turn) {
        /** Checks if white has any pawn promotions */
        promotionSquares = ((this->wPawns & Bitboard::BB_RANK_7) << 8) & ~(this->occupied);
        promotionSquares |= ((((this->wPawns << 9) & ~Bitboard::BB_FILE_A) | ((this->wPawns << 7) & ~Bitboard::BB_FILE_H)) & Bitboard::BB_RANK_8) &
                        this->bOccupied;
    } else {
        /** Checks if black has any pawn promotions */
        promotionSquares = ((this->bPawns & Bitboard::BB_RANK_2) >> 8) & ~(this->occupied);
        promotionSquares |= ((((this->bPawns >> 9) & ~Bitboard::BB_FILE_H) | ((this->bPawns >> 7) & ~Bitboard::BB_FILE_A)) & Bitboard::BB_RANK_1) &
                        this->wOccupied;
    }
    return BitUtils::popCount(promotionSquares) > 0;
}

int32_t Bitboard::pieceValue(int square) {
    piece_t piece = this->mailbox[square];
    return (1 - (piece == piece_t::EMPTY)) * SearchContext::PieceValue(piece);
}

uint64_t Bitboard::findLVA(uint64_t attadef, piece_t &piece, uint64_t target) {
    int32_t min_piece_value = INT32_MAX;
    uint64_t lva_bb = 0ULL;

    uint64_t pieces;
    uint64_t king_bb;
    int king_square;
    if (this->turn) {
        pieces = this->wOccupied;
        king_bb = this->wKing;
        king_square = this->wKingSquare;
    } else {
        pieces = this->bOccupied;
        king_bb = this->bKing;
        king_square = this->bKingSquare;
    }

    uint64_t attackmask = this->_get_attackmask(!(this->turn));
    uint64_t checkmask = this->_get_checkmask(this->turn);
    uint64_t posPinned = this->get_queen_moves(!(this->turn), king_square) & pieces;

    while (attadef) {
        int i = BitUtils::pullLSB(&attadef), value;

        uint64_t pinned_bb = Bitboard::BB_SQUARES[i] & posPinned;
        uint64_t pinmask = (pinned_bb) ? this->_get_pinmask(this->turn, i) : Bitboard::BB_ALL;
        
        if ((static_cast<int> (this->mailbox[i]) / 6) == this->turn &&
            (value = pieceValue(this->mailbox[i])) < min_piece_value && target & checkmask & pinmask) {
            min_piece_value = value;

            lva_bb = Bitboard::BB_SQUARES[i];
            piece = this->mailbox[i];
        }
    }
    return lva_bb;
}

uint64_t Bitboard::_get_attackmask(bool color) {
    uint64_t occupied;
    uint64_t key;

    uint64_t moves_bb;
    uint64_t pieces;
    int king_square;
    if (color == WHITE) {
        pieces = this->wOccupied & ~this->wPawns;
        king_square = this->bKingSquare;
        moves_bb = (((this->wPawns << 9) & ~Bitboard::BB_FILE_A) | ((this->wPawns << 7) & ~Bitboard::BB_FILE_H));
    } else {
        pieces = this->bOccupied & ~this->bPawns;
        king_square = this->wKingSquare;
        moves_bb = (((this->bPawns >> 9) & ~Bitboard::BB_FILE_H) | ((this->bPawns >> 7) & ~Bitboard::BB_FILE_A));
    }

    BitUtils::clearBit(&this->occupied, king_square);

    while (pieces) {
        int square = BitUtils::pullLSB(&pieces);
        piece_t piece = static_cast<piece_t> (this->mailbox[square] % 6);
        switch (piece) {
            case piece_t::BLACK_KNIGHT:
                moves_bb |= MoveGen::BB_KNIGHT_ATTACKS[square];
                break;
            case piece_t::BLACK_BISHOP:
                occupied = this->occupied & MoveGen::BB_BISHOP_ATTACK_MASKS[square];
                key = (occupied * MoveGen::BISHOP_MAGICS[square]) >> MoveGen::BISHOP_ATTACK_SHIFTS[square];
                moves_bb |= MoveGen::BB_BISHOP_ATTACKS[square][key];
                break;
            case piece_t::BLACK_ROOK:
                occupied = this->occupied & MoveGen::BB_ROOK_ATTACK_MASKS[square];
                key = (occupied * MoveGen::ROOK_MAGICS[square]) >> MoveGen::ROOK_ATTACK_SHIFTS[square];
                moves_bb |= MoveGen::BB_ROOK_ATTACKS[square][key];
                break;
            case piece_t::BLACK_QUEEN:
                occupied = this->occupied & MoveGen::BB_BISHOP_ATTACK_MASKS[square];
                key = (occupied * MoveGen::BISHOP_MAGICS[square]) >> MoveGen::BISHOP_ATTACK_SHIFTS[square];
                moves_bb |= MoveGen::BB_BISHOP_ATTACKS[square][key];

                occupied = this->occupied & MoveGen::BB_ROOK_ATTACK_MASKS[square];
                key = (occupied * MoveGen::ROOK_MAGICS[square]) >> MoveGen::ROOK_ATTACK_SHIFTS[square];
                moves_bb |= MoveGen::BB_ROOK_ATTACKS[square][key];
                break;
            case piece_t::BLACK_KING:
                moves_bb |= MoveGen::BB_KING_ATTACKS[square];
                break;
        }
    }
    BitUtils::setBit(&this->occupied, king_square);
    return moves_bb;
}

uint64_t Bitboard::_get_checkmask(bool color) {
    int num_attackers = 0;
    uint64_t checkmask = 0;

    int king_square;
    uint64_t enemy_bq_bb;
    uint64_t enemy_rq_bb;
    uint64_t enemy_knight_bb;
    uint64_t pawns;
    if (color == WHITE) {
        king_square = this->wKingSquare;
        enemy_bq_bb = this->bBishops | this->bQueens;
        enemy_rq_bb = this->bRooks | this->bQueens;
        enemy_knight_bb = this->bKnights;
        pawns = ((((this->wKing << 9) & ~Bitboard::BB_FILE_A) | ((this->wKing << 7) & ~Bitboard::BB_FILE_H))
                 & this->bPawns);
    } else {
        king_square = this->bKingSquare;
        enemy_bq_bb = this->wBishops | this->wQueens;
        enemy_rq_bb = this->wRooks | this->wQueens;
        enemy_knight_bb = this->wKnights;
        pawns = ((((this->bKing >> 9) & ~Bitboard::BB_FILE_H) | ((this->bKing >> 7) & ~Bitboard::BB_FILE_A))
                 & this->wPawns);
    }

    num_attackers += BitUtils::popCount(pawns);
    if (num_attackers >= 2) {
        return 0;
    }
    checkmask |= pawns;

    uint64_t rq = this->get_rook_moves(color, king_square) & enemy_rq_bb;
    num_attackers += BitUtils::popCount(rq);
    if (num_attackers >= 2) {
        return 0;
    }
    while (rq) {
        int rq_square = BitUtils::pullLSB(&rq);
        checkmask |= Bitboard::getRayBetween(king_square, rq_square);
    }

    uint64_t bq = this->get_bishop_moves(color, king_square) & enemy_bq_bb;
    num_attackers += BitUtils::popCount(bq);
    if (num_attackers >= 2) {
        return 0;
    }
    while (bq) {
        int bq_square = BitUtils::pullLSB(&bq);
        checkmask |= Bitboard::getRayBetween(king_square, bq_square);
    }

    uint64_t knights = this->get_knight_moves(color, king_square) & enemy_knight_bb;
    num_attackers += BitUtils::popCount(knights);
    if (num_attackers >= 2) {
        return 0;
    }
    checkmask |= knights;

    if (num_attackers == 0) return Bitboard::BB_ALL;
    return checkmask;
}

uint64_t Bitboard::_get_pinmask(bool color, int square) {
    uint64_t pinmask = 0;

    int king_square;
    uint64_t enemy_rq_bb;
    uint64_t enemy_bq_bb;
    if (color == WHITE) {
        king_square = this->wKingSquare;
        enemy_rq_bb = this->bRooks | this->bQueens;
        enemy_bq_bb = this->bBishops | this->bQueens;
    } else {
        king_square = this->bKingSquare;
        enemy_rq_bb = this->wRooks | this->wQueens;
        enemy_bq_bb = this->wBishops | this->wQueens;
    }

    uint64_t occupied = this->occupied & MoveGen::BB_ROOK_ATTACK_MASKS[square];
    uint64_t key = (occupied * MoveGen::ROOK_MAGICS[square]) >> MoveGen::ROOK_ATTACK_SHIFTS[square];
    uint64_t rook_attacks = MoveGen::BB_ROOK_ATTACKS[square][key];

    occupied = this->occupied & MoveGen::BB_BISHOP_ATTACK_MASKS[square];
    key = (occupied * MoveGen::BISHOP_MAGICS[square]) >> MoveGen::BISHOP_ATTACK_SHIFTS[square];
    uint64_t bishop_attacks = MoveGen::BB_BISHOP_ATTACKS[square][key];

    uint64_t direction = Bitboard::getRayBetweenInclusive(king_square, square);

    uint64_t pin = direction & rook_attacks;
    if (pin & enemy_rq_bb) {
        pinmask |= pin;
    } else {
        pin = direction & bishop_attacks;
        if (pin & enemy_bq_bb) {
            pinmask |= pin;
        }
    }
    if (pinmask) return pinmask;
    return Bitboard::BB_ALL;
}

MoveFlags Bitboard::getFlag(piece_t piece, int from, int to) {
    switch (piece) {
        case piece_t::BLACK_PAWN:
            if (to == this->en_passant_square) return MoveFlags::EN_PASSANT;
        case piece_t::BLACK_KING:
            if (abs(Bitboard::fileOf(from) - Bitboard::fileOf(to)) == 2) return MoveFlags::CASTLING;
    }
    if (Bitboard::BB_SQUARES[to] & this->occupied) return MoveFlags::CAPTURE;
    return MoveFlags::NONE;
}

uint64_t Bitboard::get_king_moves(bool color, int square) {
    uint64_t moves = MoveGen::BB_KING_ATTACKS[square];
    if (color == WHITE) {
        if (this->wKingsideCastleRights) BitUtils::setBit(&moves, Squares::G1);
        if (this->wQueensideCastleRights) BitUtils::setBit(&moves, Squares::C1);
        return moves & ~(this->wOccupied);
    } else {
        if (this->bKingsideCastleRights) BitUtils::setBit(&moves, Squares::G8);
        if (this->bQueensideCastleRights) BitUtils::setBit(&moves, Squares::C8);
        return moves & ~(this->bOccupied);
    }
}

uint64_t Bitboard::get_king_moves_no_castle(bool color, int square) {
    uint64_t moves = MoveGen::BB_KING_ATTACKS[square];
    if (color == WHITE) {
        return moves & ~(this->wOccupied);
    } else {
        return moves & ~(this->bOccupied);
    }
}

uint64_t Bitboard::get_queen_moves(bool color, int square) {
    uint64_t bishop_occupied = this->occupied & MoveGen::BB_BISHOP_ATTACK_MASKS[square];
    uint64_t bishop_key = (bishop_occupied * MoveGen::BISHOP_MAGICS[square]) >> MoveGen::BISHOP_ATTACK_SHIFTS[square];
    uint64_t bishop_moves = MoveGen::BB_BISHOP_ATTACKS[square][bishop_key];

    uint64_t rook_occupied = this->occupied & MoveGen::BB_ROOK_ATTACK_MASKS[square];
    uint64_t rook_key = (rook_occupied * MoveGen::ROOK_MAGICS[square]) >> MoveGen::ROOK_ATTACK_SHIFTS[square];
    uint64_t rook_moves = MoveGen::BB_ROOK_ATTACKS[square][rook_key];

    uint64_t moves = bishop_moves | rook_moves;

    return moves & ~(this->wOccupied * color + this->bOccupied * !color);
}

uint64_t Bitboard::get_rook_moves(bool color, int square) {
    uint64_t occupied = this->occupied & MoveGen::BB_ROOK_ATTACK_MASKS[square];
    uint64_t key = (occupied * MoveGen::ROOK_MAGICS[square]) >> MoveGen::ROOK_ATTACK_SHIFTS[square];
    uint64_t moves = MoveGen::BB_ROOK_ATTACKS[square][key];
    return moves & ~(this->wOccupied * color + this->bOccupied * !color);
}

uint64_t Bitboard::get_bishop_moves(bool color, int square) {
    uint64_t occupied = this->occupied & MoveGen::BB_BISHOP_ATTACK_MASKS[square];
    uint64_t key = (occupied * MoveGen::BISHOP_MAGICS[square]) >> MoveGen::BISHOP_ATTACK_SHIFTS[square];
    uint64_t moves = MoveGen::BB_BISHOP_ATTACKS[square][key];
    return moves & ~(this->wOccupied * color + this->bOccupied * !color);
}

uint64_t Bitboard::get_knight_moves(bool color, int square) {
    uint64_t moves = MoveGen::BB_KNIGHT_ATTACKS[square];
    return moves & ~(this->wOccupied * color + this->bOccupied * !color);
}

uint64_t Bitboard::get_pawn_moves(bool color, int square) {
    if (color == WHITE) {
        uint64_t pawn = Bitboard::BB_SQUARES[square];

        uint64_t single_push = (pawn << 8) & ~this->occupied;
        uint64_t double_push = ((single_push & Bitboard::BB_RANK_3) << 8) & ~this->occupied;

        uint64_t captures = (((pawn << 9) & ~Bitboard::BB_FILE_A) | ((pawn << 7) & ~Bitboard::BB_FILE_H))
                            & this->bOccupied;

        if (this->en_passant_square != INVALID && Bitboard::rankOf(square) + 1 == 5) {
            uint64_t ep_capture = (((pawn << 9) & ~Bitboard::BB_FILE_A) | ((pawn << 7) & ~Bitboard::BB_FILE_H))
                                  & Bitboard::BB_SQUARES[this->en_passant_square];
            captures |= ep_capture;
        }
        return single_push | double_push | captures;
    } else {
        uint64_t pawn = Bitboard::BB_SQUARES[square];

        uint64_t single_push = (pawn >> 8) & ~(this->occupied);
        uint64_t double_push = ((single_push & Bitboard::BB_RANK_6) >> 8) & ~(this->occupied);

        uint64_t captures = (((pawn >> 9) & ~Bitboard::BB_FILE_H) | ((pawn >> 7) & ~Bitboard::BB_FILE_A))
                            & this->wOccupied;

        if (this->en_passant_square != INVALID && Bitboard::rankOf(square) + 1 == 4) {
            uint64_t ep_capture = (((pawn >> 9) & ~Bitboard::BB_FILE_H) | ((pawn >> 7) & ~Bitboard::BB_FILE_A))
                                  & Bitboard::BB_SQUARES[this->en_passant_square];
            captures |= ep_capture;
        }
        return single_push | double_push | captures;
    }
}

uint64_t Bitboard::considerXrayAttacks(int from, int to) {
    int shiftAmt;
    uint64_t iterator = Bitboard::BB_SQUARES[from], boundary;
    if (from > to) {
        // Left-Shift
        bool topLeft = Bitboard::fileOf(from) < Bitboard::fileOf(to);
        bool top = Bitboard::fileOf(from) == Bitboard::fileOf(to);
        bool right = Bitboard::rankOf(from) == Bitboard::rankOf(to);
        bool topRight = (Bitboard::fileOf(from) > Bitboard::fileOf(to)) & (Bitboard::rankOf(from) > Bitboard::rankOf(to));
        shiftAmt = 7 * topLeft + 8 * top + right + 9 * topRight;
        boundary = ((topRight | right) * Bitboard::BB_FILE_A) + (topLeft * Bitboard::BB_FILE_H);
        do {
            iterator = (iterator << shiftAmt) & ~boundary;
            if (iterator & this->occupied)
                return iterator;
        } while (iterator);
    } else {
        // Right-Shift
        bool bottomRight = Bitboard::fileOf(from) > Bitboard::fileOf(to);
        bool bottom = Bitboard::fileOf(from) == Bitboard::fileOf(to);
        bool left = Bitboard::rankOf(from) == Bitboard::rankOf(to);
        bool bottomLeft = ((Bitboard::fileOf(from) < Bitboard::fileOf(to)) & (Bitboard::rankOf(from) < Bitboard::rankOf(to)));
        shiftAmt = left + 7 * bottomRight + 8 * bottom + 9 * bottomLeft;
        boundary = (Bitboard::BB_FILE_A * bottomRight) + (Bitboard::BB_FILE_H * (left | bottomLeft));
        do {
            iterator = (iterator >> shiftAmt) & ~boundary;
            if (iterator & this->occupied)
                return iterator;
        } while (iterator);
    }
    return 0ULL;
}

void Bitboard::clearBb(uint64_t from_bb) {
    this->occupied &= ~from_bb;
    this->wOccupied &= ~from_bb;
    this->bOccupied &= ~from_bb;
    this->wPawns &= ~from_bb;
    this->wKnights &= ~from_bb;
    this->wBishops &= ~from_bb;
    this->wRooks &= ~from_bb;
    this->wQueens &= ~from_bb;
    this->wKing &= ~from_bb;
    this->bPawns &= ~from_bb;
    this->bKnights &= ~from_bb;
    this->bBishops &= ~from_bb;
    this->bRooks &= ~from_bb;
    this->bQueens &= ~from_bb;
    this->bKing &= ~from_bb;
}

int32_t Bitboard::fastSEE(move_t move) {
    const Bitboard copy = *this;
    int gain[32], d = 0;
    uint64_t maxXray = this->wPawns | this->bPawns | this->wBishops | this->bBishops | this->wRooks | this->bRooks | this->wQueens | this->bQueens;
    uint64_t fromBb = Bitboard::BB_SQUARES[move.from];
    uint64_t attadef = this->attacksTo(move.to);
    gain[d] = (this->pieceValue(move.to) * (this->mailbox[move.to] != piece_t::EMPTY)) + Weights::MATERIAL[piece_t::BLACK_PAWN] * (move.flag == EN_PASSANT);
    
    piece_t attackingPiece = this->mailbox[move.from];
    uint64_t bbTo = Bitboard::BB_SQUARES[move.to];
    do {
        ++d;
        gain[d] = pieceValue(attackingPiece) - gain[d - 1];
        if (std::max(-gain[d - 1], gain[d]) < 0) break;
        attadef ^= fromBb;
        this->clearBb(fromBb);
        if (fromBb & maxXray) {
            attadef |= considerXrayAttacks(BitUtils::getLSB(fromBb), move.to);
        }

        // Add king attacks to the static exchange evaluation
        int king = (!this->turn * BitUtils::getLSB(this->wKing)) + (this->turn * BitUtils::getLSB(this->bKing));
        if (king != -1 && (bbTo & MoveGen::BB_KING_ATTACKS[king])) {
            uint64_t attackmask = this->_get_attackmask(this->turn);
            bool king_can_reach = (MoveGen::BB_KING_ATTACKS[king] & bbTo) != 0;
            bool square_undefended = (bbTo & attackmask) == 0;
            attadef |= Bitboard::BB_SQUARES[king] * (king_can_reach & square_undefended);
        }

        this->turn = !this->turn;
        fromBb = findLVA(attadef, attackingPiece, Bitboard::BB_SQUARES[move.to]);
    } while (fromBb);
    while (--d) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }
    *this = copy;
    return gain[0];
}

/**
 *
 */
uint64_t Bitboard::attacksTo(int targetSquare) {
    uint64_t attacks = 0ULL;
    uint64_t pieces = this->occupied;

    const uint64_t targetBb = Bitboard::BB_SQUARES[targetSquare];
    const uint64_t attackmasks[2] = { this->_get_attackmask(BLACK), this->_get_attackmask(WHITE)};
    while (pieces) {
        int i = BitUtils::pullLSB(&pieces);
        int oppositeColor = (this->mailbox[i] < 6);
        piece_t pieceType = static_cast<piece_t> ((static_cast<int> (this->mailbox[i]) % 6));
        switch (pieceType) {
            case BLACK_PAWN: {
                int pawnFile = Bitboard::fileOf(i), pawn_rank = Bitboard::rankOf(i);
                int targetFile = Bitboard::fileOf(targetSquare), target_rank = Bitboard::rankOf(targetSquare);
                bool areFilesAdjacent = abs(pawnFile - targetFile) == 1;
                bool isDirectionCorrect = (target_rank - pawn_rank) == (1 - (2 * (this->mailbox[i] < 6)));
                uint64_t result = areFilesAdjacent & isDirectionCorrect;
                attacks |= (result << i);
                break;
            }
            case BLACK_KNIGHT: {
                bool isAttacking = (MoveGen::BB_KNIGHT_ATTACKS[i] & targetBb) != 0;
                attacks |= (uint64_t(isAttacking) << i);
                break;
            }
            case BLACK_BISHOP: {
                uint64_t bishopAttacks = (Bitboard::BB_DIAGONALS[Bitboard::diagonalOf(i)] ^
                                           Bitboard::BB_ANTI_DIAGONALS[Bitboard::antiDiagonalOf(i)]);
                uint64_t ray = Bitboard::getRayBetween(i, targetSquare) & ~Bitboard::BB_SQUARES[i] & ~Bitboard::BB_SQUARES[targetSquare];
                bool alignmentExists = (bishopAttacks | targetBb) == bishopAttacks;
                bool obstructionFree = (ray ^ this->occupied) == (ray | this->occupied);
                uint64_t result = alignmentExists & obstructionFree;
                attacks |= (result << i);
                break;
            }
            case BLACK_ROOK: {
                uint64_t rook_attacks = (Bitboard::BB_RANKS[Bitboard::rankOf(i)] ^ Bitboard::BB_FILES[Bitboard::fileOf(i)]);
                uint64_t ray = Bitboard::getRayBetween(i, targetSquare) & ~Bitboard::BB_SQUARES[i] & ~Bitboard::BB_SQUARES[targetSquare];
                bool alignmentExists = ((rook_attacks | targetBb) == rook_attacks);
                bool obstructionFree = (ray ^ this->occupied) == (ray | this->occupied);
                uint64_t result = alignmentExists & obstructionFree;
                attacks |= (result << i);
                break;
            }
            case BLACK_QUEEN: {
                uint64_t ray = Bitboard::getRayBetween(targetSquare, i);
                bool alignmentExists = ray != 0;
                ray &= ~Bitboard::BB_SQUARES[i];
                ray &= ~Bitboard::BB_SQUARES[targetSquare];
                bool obstructionFree = (ray ^ this->occupied) == (ray | this->occupied);
                uint64_t result = alignmentExists & obstructionFree;
                attacks |= (result << i);
                break;
            }
            case BLACK_KING: {
                bool canKingReach = (MoveGen::BB_KING_ATTACKS[i] & targetBb & ~attackmasks[oppositeColor]) != 0;
                uint64_t result = canKingReach;
                attacks |= (result << i);
                break;
            }
            default:
                std::cout << "Something is seriously wrong in attacks_to() in bitboard.cpp\n";
                exit(0);
        }
    }
    return attacks;
}


/**
 * @param piece
 * @return a pointer to the bitboard of the piece.
 */
uint64_t *Bitboard::getBitboard(const piece_t &piece) {
    switch (piece) {
        case WHITE_PAWN:
            return &this->wPawns;
        case WHITE_KNIGHT:
            return &this->wKnights;
        case WHITE_BISHOP:
            return &this->wBishops;
        case WHITE_ROOK:
            return &this->wRooks;
        case WHITE_QUEEN:
            return &this->wQueens;
        case WHITE_KING:
            return &this->wKing;
        case BLACK_PAWN:
            return &this->bPawns;
        case BLACK_KNIGHT:
            return &this->bKnights;
        case BLACK_BISHOP:
            return &this->bBishops;
        case BLACK_ROOK:
            return &this->bRooks;
        case BLACK_QUEEN:
            return &this->bQueens;
        case BLACK_KING:
            return &this->bKing;
        default:
            return nullptr;
    }
}

const piece_t *Bitboard::getMailboxBoard() const {
    return this->mailbox;
}

uint64_t Bitboard::getHashCode() {
    return this->hash_code;
}

int Bitboard::getHalfmoveClock() {
    return this->halfmove_clock;
}

bool Bitboard::getTurn() const {
    return this->turn;
}

piece_t Bitboard::lookupMailbox(int square) {
    return this->mailbox[square];
}

/**
 * Prints the labeled representation of the mailbox board.
 */
void Bitboard::printBoard() {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            std::cout << ConversionUtils::toChar(this->mailbox[8 * rank + file]) << ' ';
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

bool Bitboard::operator==(const Bitboard &other) {
    return
        (this->wPawns == other.wPawns) && 
        (this->wKnights == other.wKnights) &&
        (this->wBishops == other.wBishops) &&
        (this->wRooks == other.wRooks) &&
        (this->wQueens == other.wQueens) &&
        (this->wKing == other.wKing) &&
        (this->bPawns = other.bPawns) &&
        (this->bKnights = other.bKnights) &&
        (this->bBishops == other.bBishops) &&
        (this->bRooks == other.bRooks) &&
        (this->bQueens == other.bQueens) &&
        (this->bKing == other.bKing) &&
        (this->turn == other.turn) &&
        (this->wKingsideCastleRights == other.wKingsideCastleRights) &&
        (this->wQueensideCastleRights == other.wQueensideCastleRights) &&
        (this->bKingsideCastleRights == other.bKingsideCastleRights) &&
        (this->bQueensideCastleRights == other.bQueensideCastleRights);
}

void Bitboard::operator=(const Bitboard &other) {
    std::memcpy(this->mailbox, &(other.mailbox), 64 * sizeof(piece_t));
    this->wPawns = other.wPawns;
    this->wKnights = other.wKnights;
    this->wBishops = other.wBishops;
    this->wRooks = other.wRooks;
    this->wQueens = other.wQueens;
    this->wKing = other.wKing;

    this->bPawns = other.bPawns;
    this->bKnights = other.bKnights;
    this->bBishops = other.bBishops;
    this->bRooks = other.bRooks;
    this->bQueens = other.bQueens;
    this->bKing = other.bKing;

    this->occupied = other.occupied;
    this->wOccupied = other.wOccupied;
    this->bOccupied = other.bOccupied;

    this->wKingSquare = other.wKingSquare;
    this->bKingSquare = other.bKingSquare;
    
    this->turn = other.turn;

    this->wKingsideCastleRights = other.wKingsideCastleRights;
    this->wQueensideCastleRights = other.wQueensideCastleRights;

    this->bKingsideCastleRights = other.bKingsideCastleRights;
    this->bQueensideCastleRights = other.bQueensideCastleRights;

    this->en_passant_square = other.en_passant_square;
    this->halfmove_clock = other.halfmove_clock;
    this->fullmove_number = other.fullmove_number;
    this->hash_code = other.hash_code;
}