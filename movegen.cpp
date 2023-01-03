#include <iostream>

#include "stack.h"
#include "weights.h"
#include "movegen.h"
#include "bitboard.h"
#include "search.h"


extern bitboard board;

// Pseudo-legal bitboards indexed by square to determine where that piece can attack
const uint64_t BB_KNIGHT_ATTACKS[64] = {
        0x20400, 0x50800, 0xa1100, 0x142200, 0x284400,
        0x508800, 0xa01000, 0x402000, 0x2040004, 0x5080008,
        0xa110011, 0x14220022, 0x28440044, 0x50880088, 0xa0100010,
        0x40200020, 0x204000402, 0x508000805, 0xa1100110a, 0x1422002214,
        0x2844004428, 0x5088008850, 0xa0100010a0, 0x4020002040, 0x20400040200,
        0x50800080500, 0xa1100110a00, 0x142200221400, 0x284400442800, 0x508800885000,
        0xa0100010a000, 0x402000204000, 0x2040004020000, 0x5080008050000, 0xa1100110a0000,
        0x14220022140000, 0x28440044280000, 0x50880088500000, 0xa0100010a00000, 0x40200020400000,
        0x204000402000000, 0x508000805000000, 0xa1100110a000000, 0x1422002214000000, 0x2844004428000000,
        0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000, 0x400040200000000, 0x800080500000000,
        0x1100110a00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010a000000000,
        0x2000204000000000, 0x4020000000000, 0x8050000000000, 0x110a0000000000, 0x22140000000000,
        0x44280000000000, 0x88500000000000, 0x10a00000000000, 0x20400000000000
};

uint64_t BB_BISHOP_ATTACKS[64][512];

uint64_t BB_ROOK_ATTACKS[64][4096];

const uint64_t BB_KING_ATTACKS[64] = {
        0x302, 0x705, 0xe0a, 0x1c14, 0x3828,
        0x7050, 0xe0a0, 0xc040, 0x30203, 0x70507,
        0xe0a0e, 0x1c141c, 0x382838, 0x705070, 0xe0a0e0,
        0xc040c0, 0x3020300, 0x7050700, 0xe0a0e00, 0x1c141c00,
        0x38283800, 0x70507000, 0xe0a0e000, 0xc040c000, 0x302030000,
        0x705070000, 0xe0a0e0000, 0x1c141c0000, 0x3828380000, 0x7050700000,
        0xe0a0e00000, 0xc040c00000, 0x30203000000, 0x70507000000, 0xe0a0e000000,
        0x1c141c000000, 0x382838000000, 0x705070000000, 0xe0a0e0000000, 0xc040c0000000,
        0x3020300000000, 0x7050700000000, 0xe0a0e00000000, 0x1c141c00000000, 0x38283800000000,
        0x70507000000000, 0xe0a0e000000000, 0xc040c000000000, 0x302030000000000, 0x705070000000000,
        0xe0a0e0000000000, 0x1c141c0000000000, 0x3828380000000000, 0x7050700000000000, 0xe0a0e00000000000,
        0xc040c00000000000, 0x203000000000000, 0x507000000000000, 0xa0e000000000000, 0x141c000000000000,
        0x2838000000000000, 0x5070000000000000, 0xa0e0000000000000, 0x40c0000000000000,
};


// Rook and bishop magic numbers to generate their magic bitboards
const uint64_t BISHOP_MAGICS[64] = {
        0x2020202020200, 0x2020202020000, 0x4010202000000, 0x4040080000000, 0x1104000000000,
        0x821040000000, 0x410410400000, 0x104104104000, 0x40404040400, 0x20202020200,
        0x40102020000, 0x40400800000, 0x11040000000, 0x8210400000, 0x4104104000,
        0x2082082000, 0x4000808080800, 0x2000404040400, 0x1000202020200, 0x800802004000,
        0x800400a00000, 0x200100884000, 0x400082082000, 0x200041041000, 0x2080010101000,
        0x1040008080800, 0x208004010400, 0x404004010200, 0x840000802000, 0x404002011000,
        0x808001041000, 0x404000820800, 0x1041000202000, 0x820800101000, 0x104400080800,
        0x20080080080, 0x404040040100, 0x808100020100, 0x1010100020800, 0x808080010400,
        0x820820004000, 0x410410002000, 0x82088001000, 0x2011000800, 0x80100400400,
        0x1010101000200, 0x2020202000400, 0x1010101000200, 0x410410400000, 0x208208200000,
        0x2084100000, 0x20880000, 0x1002020000, 0x40408020000, 0x4040404040000,
        0x2020202020000, 0x104104104000, 0x2082082000, 0x20841000, 0x208800,
        0x10020200, 0x404080200, 0x40404040400, 0x2020202020200
};

const uint64_t ROOK_MAGICS[64] = {
        0x80001020400080, 0x40001000200040, 0x80081000200080, 0x80040800100080, 0x80020400080080,
        0x80010200040080, 0x80008001000200, 0x80002040800100, 0x800020400080, 0x400020005000,
        0x801000200080, 0x800800100080, 0x800400080080, 0x800200040080, 0x800100020080,
        0x800040800100, 0x208000400080, 0x404000201000, 0x808010002000, 0x808008001000,
        0x808004000800, 0x808002000400, 0x10100020004, 0x20000408104, 0x208080004000,
        0x200040005000, 0x100080200080, 0x80080100080, 0x40080080080, 0x20080040080,
        0x10080800200, 0x800080004100, 0x204000800080, 0x200040401000, 0x100080802000,
        0x80080801000, 0x40080800800, 0x20080800400, 0x20001010004, 0x800040800100,
        0x204000808000, 0x200040008080, 0x100020008080, 0x80010008080, 0x40008008080,
        0x20004008080, 0x10002008080, 0x4081020004, 0x204000800080, 0x200040008080,
        0x100020008080, 0x80010008080, 0x40008008080, 0x20004008080, 0x800100020080,
        0x800041000080, 0xfffcddfced714a, 0x7ffcddfced714a, 0x3fffcdffd88096, 0x40810002101,
        0x1000204080011, 0x1000204000801, 0x1000082000401, 0x1fffaabfad1a2
};

// Attack masks and shifts for magic bitboard move generation
uint64_t BB_BISHOP_ATTACK_MASKS[64];
uint64_t BB_ROOK_ATTACK_MASKS[64];
uint64_t ROOK_ATTACK_SHIFTS[64];
uint64_t BISHOP_ATTACK_SHIFTS[64];

/**
 * Initalizes the bishop attack magic bitboard
 * @author github.com/nkarve
 */
void init_bishop_attacks() {
    for (int square = A1; square <= H8; square++) {
        uint64_t edges = ((BB_RANK_1 | BB_RANK_8) & ~BB_RANKS[rank_of(square)]) |
                         ((BB_FILE_A | BB_FILE_H) & ~BB_FILES[file_of(square)]);
        BB_BISHOP_ATTACK_MASKS[square] = (BB_DIAGONALS[diagonal_of(square)] ^ BB_ANTI_DIAGONALS[anti_diagonal_of(square)]) & ~edges;
        uint64_t attack_mask = BB_BISHOP_ATTACK_MASKS[square];

        int shift = 64 - pop_count(attack_mask);
        BISHOP_ATTACK_SHIFTS[square] = shift;

        uint64_t subset = 0;
        do {
            uint64_t index = subset;
            index *= BISHOP_MAGICS[square];
            index >>= shift;
            BB_BISHOP_ATTACKS[square][index] = _init_bishop_attacks_helper(square, subset);
            subset = (subset - attack_mask) & attack_mask;
        } while (subset);
    }
}


/**
 * Initalizes the rook attack magic bitboard
 * @author github.com/nkarve
 */
void init_rook_attacks() {
    for (int square = A1; square <= H8; square++) {
        uint64_t edges = ((BB_RANK_1 | BB_RANK_8) & ~BB_RANKS[rank_of(square)]) |
                         ((BB_FILE_A | BB_FILE_H) & ~BB_FILES[file_of(square)]);
        BB_ROOK_ATTACK_MASKS[square] = (BB_RANKS[rank_of(square)] ^ BB_FILES[file_of(square)]) & ~edges;
        uint64_t attack_mask = BB_ROOK_ATTACK_MASKS[square];

        int shift = 64 - pop_count(attack_mask);
        ROOK_ATTACK_SHIFTS[square] = shift;

        uint64_t subset = 0;
        do {
            uint64_t index = subset;
            index *= ROOK_MAGICS[square];
            index >>= shift;
            BB_ROOK_ATTACKS[square][index] = _init_rook_attacks_helper(square, subset);
            subset = (subset - attack_mask) & attack_mask;
        } while (subset);
    }
}

void _init_rays() {
    for (int square1 = A1; square1 <= H8; square1++) {
        for (int square2 = A1; square2 <= H8; square2++) {
            if (square1 == square2) {
                BB_RAYS[square1][square2] = 0;
                continue;
            }

            uint64_t square2_bb = BB_SQUARES[square2];

            uint64_t rank = BB_RANKS[rank_of(square1)];
            if (rank & square2_bb) {
                BB_RAYS[square1][square2] = rank;
                continue;
            }

            uint64_t file = BB_FILES[file_of(square1)];
            if (file & square2_bb) {
                BB_RAYS[square1][square2] = file;
                continue;
            }

            uint64_t diagonal = BB_DIAGONALS[diagonal_of(square1)];
            if (diagonal & square2_bb) {
                BB_RAYS[square1][square2] = diagonal;
                continue;
            }

            uint64_t anti_diagonal = BB_ANTI_DIAGONALS[anti_diagonal_of(square1)];
            if (anti_diagonal & square2_bb) {
                BB_RAYS[square1][square2] = anti_diagonal;
                continue;
            }

            BB_RAYS[square1][square2] = 0;
        }
    }
}


/**
 * Helper method to initalizes the bishop attack magic bitboard
 * @param square the current square
 * @param subset the current occupancy
 * @param attack_mask the bishop's attack mask without edges
 * @return the bishop attack bitboard
 * @author github.com/nkarve
 */
static uint64_t _init_bishop_attacks_helper(int square, uint64_t subset) {
    uint64_t square_mask = BB_SQUARES[square];
    uint64_t diagonal_mask = BB_DIAGONALS[diagonal_of(square)];
    uint64_t anti_diagonal_mask = BB_ANTI_DIAGONALS[anti_diagonal_of(square)];

    uint64_t diagonal_attacks = (((diagonal_mask & subset) - square_mask * 2) ^
                                 _get_reverse_bb(_get_reverse_bb(diagonal_mask & subset) - _get_reverse_bb(square_mask) * 2)) &
                                diagonal_mask;

    uint64_t anti_diagonal_attacks = (((anti_diagonal_mask & subset) - square_mask * 2) ^
                                      _get_reverse_bb(_get_reverse_bb(anti_diagonal_mask & subset) - _get_reverse_bb(square_mask) * 2)) &
                                     anti_diagonal_mask;

    return diagonal_attacks | anti_diagonal_attacks;
}


/**
 * Helper method to initalizes the rook attack magic bitboard
 * @param square the current square
 * @param subset the current occupancy
 * @param attack_mask the rook's attack mask without edges
 * @return the rook attack bitboard
 * @author github.com/nkarve
 */
static uint64_t _init_rook_attacks_helper(int square, uint64_t subset) {
    uint64_t square_mask = BB_SQUARES[square];
    uint64_t rank_mask = BB_RANKS[rank_of(square)];
    uint64_t file_mask = BB_FILES[file_of(square)];

    uint64_t rank_attacks = (((rank_mask & subset) - square_mask * 2) ^
                             _get_reverse_bb(_get_reverse_bb(rank_mask & subset) - _get_reverse_bb(square_mask) * 2)) &
                            rank_mask;

    uint64_t file_attacks = (((file_mask & subset) - square_mask * 2) ^
                             _get_reverse_bb(_get_reverse_bb(file_mask & subset) - _get_reverse_bb(square_mask) * 2)) &
                            file_mask;

    return rank_attacks | file_attacks;
}


/**
 * @param bb
 * @return the reverse of the bitboard. Flips the perspective of the board
 * @author github.com/nkarve
 */
static uint64_t _get_reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

/**
 * Takes in an empty array and generates the list of legal moves in it.
 * @param moves the array to store the moves in.
 * @param color the side to move.
 * @param return the number of moves.
 */
int gen_legal_moves(move_t* moves, bool color) {
    int i = 0;
    uint64_t pieces;
    uint64_t king_bb;
    int king_square;
    uint64_t enemy_pawns_attacks;
    if (color == WHITE) {
        pieces = board.w_occupied;
        king_bb = board.w_king;
        king_square = board.w_king_square;
        enemy_pawns_attacks = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A))
                              & board.w_occupied;
    } else {
        pieces = board.b_occupied;
        king_bb = board.b_king;
        king_square = board.b_king_square;
        enemy_pawns_attacks = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H))
                              & board.b_occupied;
    }

    uint64_t attackmask = _get_attackmask(!color);
    uint64_t checkmask = _get_checkmask(color);
    uint64_t pos_pinned = get_queen_moves(!color, king_square) & pieces;

    // King is in double check, only moves are to move king away
    if (!checkmask) {
        uint64_t moves_bb = get_king_moves(color, king_square) & ~attackmask;
        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            int flag = get_flag('K', king_square, to);
            if (flag == CASTLING) continue;
            move_t move = {(unsigned int) king_square, (unsigned int) to, (unsigned int) flag};
            moves[i++] = move;
        }
        return i;
    }

    while (pieces) {
        int from = pull_lsb(&pieces);
        char piece = toupper(board.mailbox[from]);

        uint64_t pinmask;
        uint64_t pinned_bb = BB_SQUARES[from] & pos_pinned;
        if (pinned_bb) {
            pinmask = _get_pinmask(color, from);
        } else {
            pinmask = BB_ALL;
        }

        uint64_t moves_bb;
        switch (piece) {
            case 'P': {
                uint64_t pawn_moves = get_pawn_moves(color, from);
                moves_bb = pawn_moves & checkmask & pinmask;

                if (board.en_passant_square != INVALID) {
                    if (pawn_moves & pinmask & BB_SQUARES[board.en_passant_square]) {
                        // Add possible en passant capture to remove check
                        // For example en passant is legal here:
                        // 8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1
                        if (king_bb & enemy_pawns_attacks) {
                            set_bit(&moves_bb, board.en_passant_square);
                        }
                    }
                }

                break;
            }
            case 'N':
                moves_bb = get_knight_moves(color, from) & checkmask & pinmask;
                break;
            case 'B':
                moves_bb = get_bishop_moves(color, from) & checkmask & pinmask;
                break;
            case 'R':
                moves_bb = get_rook_moves(color, from) & checkmask & pinmask;
                break;
            case 'Q':
                moves_bb = get_queen_moves(color, from) & checkmask & pinmask;
                break;
            case 'K':
                moves_bb = get_king_moves(color, from) & ~attackmask;
                break;
        }

        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            if (piece == 'P' && (rank_of(to) == 0 || rank_of(to) == 7)) { // Add all promotions
                if (board.mailbox[to] == '-') {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, PR_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, PR_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, PR_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, PR_KNIGHT};
                    moves[i++] = knight_promotion;
                } else {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, PC_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, PC_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int)  to, PC_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, PC_KNIGHT};
                    moves[i++] = knight_promotion;
                }

            } else {
                int flag = get_flag(piece, from, to);
                move_t move = {(unsigned int) from, (unsigned int) to, (unsigned int) flag};

                // Determine if castling is legal
                if (flag == CASTLING) {
                    if (attackmask & king_bb) continue; // Assert the king is not in check
                    if (color == WHITE) {
                        if (from != E1) continue; // Assert the king is still alive
                        if (to == G1) { // Kingside
                            if (!board.w_kingside_castling_rights) continue; // Assert king or rook has not moved
                            if (!(board.w_rooks & BB_SQUARES[H1])) continue; // Assert rook is still alive
                            if (board.occupied & (BB_SQUARES[F1] | BB_SQUARES[G1])) continue; // Assert there are no pieces between the king and rook
                            if (attackmask & (BB_SQUARES[F1] | BB_SQUARES[G1])) continue; // Assert the squares the king moves through are not attacked
                        } else if (to == C1) { // Queenside
                            if (!board.w_queenside_castling_rights) continue;
                            if (!(board.w_rooks & BB_SQUARES[A1])) continue;
                            if (board.occupied & (BB_SQUARES[D1] | BB_SQUARES[C1] | BB_SQUARES[B1])) continue;
                            if (attackmask & (BB_SQUARES[D1] | BB_SQUARES[C1])) continue;
                        } else {
                            continue;
                        }
                    } else {
                        if (from != E8) continue;
                        if (to == G8) { // Kingside
                            if (!board.b_kingside_castling_rights) continue;
                            if (!(board.b_rooks & BB_SQUARES[H8])) continue;
                            if (board.occupied & (BB_SQUARES[F8] | BB_SQUARES[G8])) continue;
                            if (attackmask & (BB_SQUARES[F8] | BB_SQUARES[G8])) continue;
                        } else if (to == C8) { // Queenside
                            if (!board.b_queenside_castling_rights) continue;
                            if (!(board.b_rooks & BB_SQUARES[A8])) continue;
                            if (board.occupied & (BB_SQUARES[D8] | BB_SQUARES[C8] | BB_SQUARES[B8])) continue;
                            if (attackmask & (BB_SQUARES[D8] | BB_SQUARES[C8])) continue;
                        } else {
                            continue;
                        }
                    }
                } else if (flag == EN_PASSANT) {
                    // Remove possible en passant capture that leaves king in check
                    // For example en passant is illegal here:
                    // 8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1
                    // k7/1q6/8/3pP3/8/5K2/8/8 w - d6 0 1
                    push(move);
                    bool invalid = is_check(color);
                    pop();
                    if (invalid) continue;
                }
                moves[i++] = move;
            }
        }
    }
    return i;
}


/**
 * Takes in an empty array and generates the list of legal captures in it.
 * @param moves the array to store the captures in.
 * @param color the side to move.
 * @param return the number of captures.
 */
int gen_legal_captures(move_t* moves, bool color) {
    int i = 0;

    uint64_t pieces;
    uint64_t king_bb;
    int king_square;
    uint64_t enemy_pawns_attacks;
    uint64_t enemy_bb;
    if (color == WHITE) {
        pieces = board.w_occupied;
        king_bb = board.w_king;
        king_square = board.w_king_square;
        enemy_pawns_attacks = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A))
                              & board.w_occupied;
        enemy_bb = board.b_occupied;
    } else {
        pieces = board.b_occupied;
        king_bb = board.b_king;
        king_square = board.b_king_square;
        enemy_pawns_attacks = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H))
                              & board.b_occupied;
        enemy_bb = board.w_occupied;
    }

    uint64_t attackmask = _get_attackmask(!color);
    uint64_t checkmask = _get_checkmask(color);
    uint64_t pos_pinned = get_queen_moves(!color, king_square) & pieces;

    // King is in double check, only moves are to king moves away that are captures
    if (!checkmask) {
        uint64_t moves_bb = get_king_moves(color, king_square) & ~attackmask & enemy_bb;
        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            int flag = get_flag('K', king_square, to);
            move_t move = {(unsigned int) king_square, (unsigned int) to,(unsigned int)  flag};
            moves[i++] = move;
        }
        return i;
    }

    while (pieces) {
        int from = pull_lsb(&pieces);
        char piece = toupper(board.mailbox[from]);

        uint64_t pinmask;
        uint64_t pinned_bb = BB_SQUARES[from] & pos_pinned;
        if (pinned_bb) {
            pinmask = _get_pinmask(color, from);
        } else {
            pinmask = BB_ALL;
        }

        uint64_t moves_bb;
        switch (piece) {
            case 'P': {
                uint64_t pawn_moves = get_pawn_moves(color, from);
                moves_bb = pawn_moves & checkmask & pinmask & enemy_bb;

                if (board.en_passant_square != INVALID) {
                    if (pawn_moves & pinmask & BB_SQUARES[board.en_passant_square]) {
                        // Add possible en passant capture to remove check
                        // For example en passant is legal here:
                        // 8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1
                        if (king_bb & enemy_pawns_attacks) {
                            set_bit(&moves_bb, board.en_passant_square);
                        }
                    }
                }
                break;
            }
            case 'N':
                moves_bb = get_knight_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'B':
                moves_bb = get_bishop_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'R':
                moves_bb = get_rook_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'Q':
                moves_bb = get_queen_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'K':
                moves_bb = get_king_moves(color, from) & ~attackmask & enemy_bb;
                break;
        }

        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            if (piece == 'P' && (rank_of(to) == 0 || rank_of(to) == 7)) { // Add all promotion captures
                if (board.mailbox[to] != '-') {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, PC_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, PC_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, PC_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, PC_KNIGHT};
                    moves[i++] = knight_promotion;
                }
            } else {
                int flag = get_flag(piece, from, to);
                move_t move = {(unsigned int) from, (unsigned int) to, (unsigned int) flag};

                if (flag == EN_PASSANT) {
                    // Remove possible en passant capture that leaves king in check
                    // For example en passant is illegal here:
                    // 8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1
                    // k7/1q6/8/3pP3/8/5K2/8/8 w - d6 0 1
                    push(move);
                    bool invalid = is_check(color);
                    pop();
                    if (invalid) continue;
                }
                moves[i++] = move;
            }
        }
    }
    return i;
}

int gen_legal_captures_sq(move_t *moves, bool color, uint64_t square) {
    int i = 0;

    uint64_t pieces;
    uint64_t king_bb;
    int king_square;
    uint64_t enemy_pawns_attacks;
    uint64_t enemy_bb;
    if (color == WHITE) {
        pieces = board.w_occupied;
        king_bb = board.w_king;
        king_square = board.w_king_square;
        enemy_pawns_attacks = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A))
                              & board.w_occupied;
        enemy_bb = board.b_occupied & square;
    } else {
        pieces = board.b_occupied;
        king_bb = board.b_king;
        king_square = board.b_king_square;
        enemy_pawns_attacks = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H))
                              & board.b_occupied;
        enemy_bb = board.w_occupied & square;
    }

    uint64_t attackmask = _get_attackmask(!color);
    uint64_t checkmask = _get_checkmask(color);
    uint64_t pos_pinned = get_queen_moves(!color, king_square) & pieces;

    // King is in double check, only moves are to king moves away that are captures
    if (!checkmask) {
        uint64_t moves_bb = get_king_moves(color, king_square) & ~attackmask & enemy_bb;
        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            int flag = get_flag('K', king_square, to);
            move_t move = {(unsigned int) king_square, (unsigned int) to,(unsigned int)  flag};
            moves[i++] = move;
        }
        return i;
    }

    while (pieces) {
        int from = pull_lsb(&pieces);
        char piece = toupper(board.mailbox[from]);

        uint64_t pinmask;
        uint64_t pinned_bb = BB_SQUARES[from] & pos_pinned;
        if (pinned_bb) {
            pinmask = _get_pinmask(color, from);
        } else {
            pinmask = BB_ALL;
        }

        uint64_t moves_bb;
        switch (piece) {
            case 'P': {
                uint64_t pawn_moves = get_pawn_moves(color, from);
                moves_bb = pawn_moves & checkmask & pinmask & enemy_bb;

                if (board.en_passant_square != INVALID) {
                    if (pawn_moves & pinmask & BB_SQUARES[board.en_passant_square]) {
                        // Add possible en passant capture to remove check
                        // For example en passant is legal here:
                        // 8/8/8/2k5/3Pp3/8/8/3K4 b - d3 0 1
                        if (king_bb & enemy_pawns_attacks) {
                            set_bit(&moves_bb, board.en_passant_square);
                        }
                    }
                }
                break;
            }
            case 'N':
                moves_bb = get_knight_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'B':
                moves_bb = get_bishop_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'R':
                moves_bb = get_rook_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'Q':
                moves_bb = get_queen_moves(color, from) & checkmask & pinmask & enemy_bb;
                break;
            case 'K':
                moves_bb = get_king_moves(color, from) & ~attackmask & enemy_bb;
                break;
        }

        while (moves_bb) {
            int to = pull_lsb(&moves_bb);
            if (piece == 'P' && (rank_of(to) == 0 || rank_of(to) == 7)) { // Add all promotion captures
                if (board.mailbox[to] != '-') {
                    move_t queen_promotion = {(unsigned int) from, (unsigned int) to, PC_QUEEN};
                    moves[i++] = queen_promotion;
                    move_t rook_promotion = {(unsigned int) from, (unsigned int) to, PC_ROOK};
                    moves[i++] = rook_promotion;
                    move_t bishop_promotion = {(unsigned int) from, (unsigned int) to, PC_BISHOP};
                    moves[i++] = bishop_promotion;
                    move_t knight_promotion = {(unsigned int) from, (unsigned int) to, PC_KNIGHT};
                    moves[i++] = knight_promotion;
                }
            } else {
                int flag = get_flag(piece, from, to);
                move_t move = {(unsigned int) from, (unsigned int) to, (unsigned int) flag};

                if (flag == EN_PASSANT) {
                    // Remove possible en passant capture that leaves king in check
                    // For example en passant is illegal here:
                    // 8/8/8/8/k2Pp2Q/8/8/3K4 b - d3 0 1
                    // k7/1q6/8/3pP3/8/5K2/8/8 w - d6 0 1
                    push(move);
                    bool invalid = is_check(color);
                    pop();
                    if (invalid) continue;
                }
                moves[i++] = move;
            }
        }
    }
    return i;
}

int gen_nonquiescent_moves(move_t *moves, bool color, int *n_checks) {
    int n = gen_legal_moves(moves, color);
    int num_checks = 0, num_proms = 0, num_captures = 0;
    for (int i = 0; i < n; ++i) {
        if (is_move_check(moves[i])) {
            /* move_t puts opponent in check */
            moves[num_checks + num_proms + num_captures] = moves[num_checks + num_proms];
            moves[num_checks + num_proms] = moves[num_checks];
            moves[num_checks] = moves[i];
            ++num_checks;
        } else if (moves[i].flag >= PR_KNIGHT) {
            /* move_t is a promotion */
            moves[num_checks + num_proms + num_captures] = moves[num_checks + num_proms];
            moves[num_checks + num_proms] = moves[i];
            ++num_proms;
        } else if ((moves[i].flag == CAPTURE && move_SEE(moves[i]) >= 0) || moves[i].flag == EN_PASSANT) {
            /**
             * move_t is a non-losing capture.
             */
            moves[num_checks + num_proms + num_captures] = moves[i];
            ++num_captures;
        }
    }
    *n_checks = num_checks;
    return num_checks + num_proms + num_captures;
}

/**
 * @param color the side to move
 * @param piece
 * @param from the square the piece is moving from
 * @param to the square the piece is moving to
 * @return the appropriate flag for the move, excludes promotions
 */
int get_flag(char piece, int from, int to) {
    switch (piece) {
        case 'P':
            if (to == board.en_passant_square) return EN_PASSANT;
        case 'K':
            if (abs(file_of(from) - file_of(to)) == 2) return CASTLING;
    }
    if (BB_SQUARES[to] & board.occupied) return CAPTURE;
    return NONE;
}


/**
 * @param color
 * @return the bitboard of squares the king of the color can't go.
 * All squares the color is attacking.
 */
static uint64_t _get_attackmask(bool color) {
    uint64_t occupied;
    uint64_t key;

    uint64_t moves_bb;
    uint64_t pieces;
    int king_square;
    if (color == WHITE) {
        pieces = board.w_occupied & ~board.w_pawns;
        king_square = board.b_king_square;
        moves_bb = (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H));
    } else {
        pieces = board.b_occupied & ~board.b_pawns;
        king_square = board.w_king_square;
        moves_bb = (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A));
    }

    clear_bit(&board.occupied, king_square);

    while (pieces) {
        int square = pull_lsb(&pieces);
        char piece = toupper(board.mailbox[square]);
        switch (piece) {
            case 'N':
                moves_bb |= BB_KNIGHT_ATTACKS[square];
                break;
            case 'B':
                occupied = board.occupied & BB_BISHOP_ATTACK_MASKS[square];
                key = (occupied * BISHOP_MAGICS[square]) >> BISHOP_ATTACK_SHIFTS[square];
                moves_bb |= BB_BISHOP_ATTACKS[square][key];
                break;
            case 'R':
                occupied = board.occupied & BB_ROOK_ATTACK_MASKS[square];
                key = (occupied * ROOK_MAGICS[square]) >> ROOK_ATTACK_SHIFTS[square];
                moves_bb |= BB_ROOK_ATTACKS[square][key];
                break;
            case 'Q':
                occupied = board.occupied & BB_BISHOP_ATTACK_MASKS[square];
                key = (occupied * BISHOP_MAGICS[square]) >> BISHOP_ATTACK_SHIFTS[square];
                moves_bb |= BB_BISHOP_ATTACKS[square][key];

                occupied = board.occupied & BB_ROOK_ATTACK_MASKS[square];
                key = (occupied * ROOK_MAGICS[square]) >> ROOK_ATTACK_SHIFTS[square];
                moves_bb |= BB_ROOK_ATTACKS[square][key];
                break;
            case 'K':
                moves_bb |= BB_KING_ATTACKS[square];
                break;
        }
    }
    set_bit(&board.occupied, king_square);
    return moves_bb;
}


/**
 * @param color the color of the king possibly in check.
 * @return all squares if the king is not in check, else
 * the path between the attacking piece and the color's king.
 * Returns empty bitboard if in double or more check.
 */
static uint64_t _get_checkmask(bool color) {
    int num_attackers = 0;
    uint64_t checkmask = 0;

    int king_square;
    uint64_t enemy_bq_bb;
    uint64_t enemy_rq_bb;
    uint64_t enemy_knight_bb;
    uint64_t pawns;
    if (color == WHITE) {
        king_square = board.w_king_square;
        enemy_bq_bb = board.b_bishops | board.b_queens;
        enemy_rq_bb = board.b_rooks | board.b_queens;
        enemy_knight_bb = board.b_knights;
        pawns = ((((board.w_king << 9) & ~BB_FILE_A) | ((board.w_king << 7) & ~BB_FILE_H))
                 & board.b_pawns);
    } else {
        king_square = board.b_king_square;
        enemy_bq_bb = board.w_bishops | board.w_queens;
        enemy_rq_bb = board.w_rooks | board.w_queens;
        enemy_knight_bb = board.w_knights;
        pawns = ((((board.b_king >> 9) & ~BB_FILE_H) | ((board.b_king >> 7) & ~BB_FILE_A))
                 & board.w_pawns);
    }

    num_attackers += pop_count(pawns);
    if (num_attackers >= 2) {
        return 0;
    }
    checkmask |= pawns;

    uint64_t rq = get_rook_moves(color, king_square) & enemy_rq_bb;
    num_attackers += pop_count(rq);
    if (num_attackers >= 2) {
        return 0;
    }
    while (rq) {
        int rq_square = pull_lsb(&rq);
        checkmask |= get_ray_between(king_square, rq_square);
    }

    uint64_t bq = get_bishop_moves(color, king_square) & enemy_bq_bb;
    num_attackers += pop_count(bq);
    if (num_attackers >= 2) {
        return 0;
    }
    while (bq) {
        int bq_square = pull_lsb(&bq);
        checkmask |= get_ray_between(king_square, bq_square);
    }

    uint64_t knights = get_knight_moves(color, king_square) & enemy_knight_bb;
    num_attackers += pop_count(knights);
    if (num_attackers >= 2) {
        return 0;
    }
    checkmask |= knights;

    if (num_attackers == 0) return BB_ALL;
    return checkmask;
}


/**
 * @param color
 * @param king_square
 * @param square the square the possibly pinned piece is on.
 * @return a possible pin ray for the piece.
 */
static uint64_t _get_pinmask(bool color, int square) {
    uint64_t pinmask = 0;

    int king_square;
    uint64_t enemy_rq_bb;
    uint64_t enemy_bq_bb;
    if (color == WHITE) {
        king_square = board.w_king_square;
        enemy_rq_bb = board.b_rooks | board.b_queens;
        enemy_bq_bb = board.b_bishops | board.b_queens;
    } else {
        king_square = board.b_king_square;
        enemy_rq_bb = board.w_rooks | board.w_queens;
        enemy_bq_bb = board.w_bishops | board.w_queens;
    }

    uint64_t occupied = board.occupied & BB_ROOK_ATTACK_MASKS[square];
    uint64_t key = (occupied * ROOK_MAGICS[square]) >> ROOK_ATTACK_SHIFTS[square];
    uint64_t rook_attacks = BB_ROOK_ATTACKS[square][key];

    occupied = board.occupied & BB_BISHOP_ATTACK_MASKS[square];
    key = (occupied * BISHOP_MAGICS[square]) >> BISHOP_ATTACK_SHIFTS[square];
    uint64_t bishop_attacks = BB_BISHOP_ATTACKS[square][key];

    uint64_t direction = get_ray_between_inclusive(king_square, square);

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
    return BB_ALL;
}


/**
 * @param color the color of the pawn.
 * @param square the square the pawn is on.
 * @return where the pawn can move from the given square.
 */
uint64_t get_pawn_moves(bool color, int square) {
    if (color == WHITE) {
        uint64_t pawn = BB_SQUARES[square];

        uint64_t single_push = (pawn << 8) & ~board.occupied;
        uint64_t double_push = ((single_push & BB_RANK_3) << 8) & ~board.occupied;

        uint64_t captures = (((pawn << 9) & ~BB_FILE_A) | ((pawn << 7) & ~BB_FILE_H))
                            & board.b_occupied;

        if (board.en_passant_square != INVALID && rank_of(square) + 1 == 5) {
            uint64_t ep_capture = (((pawn << 9) & ~BB_FILE_A) | ((pawn << 7) & ~BB_FILE_H))
                                  & BB_SQUARES[board.en_passant_square];
            captures |= ep_capture;
        }

        return single_push | double_push | captures;
    } else {
        uint64_t pawn = BB_SQUARES[square];

        uint64_t single_push = (pawn >> 8) & ~board.occupied;
        uint64_t double_push = ((single_push & BB_RANK_6) >> 8) & ~board.occupied;

        uint64_t captures = (((pawn >> 9) & ~BB_FILE_H) | ((pawn >> 7) & ~BB_FILE_A))
                            & board.w_occupied;

        if (board.en_passant_square != INVALID && rank_of(square) + 1 == 4) {
            uint64_t ep_capture = (((pawn >> 9) & ~BB_FILE_H) | ((pawn >> 7) & ~BB_FILE_A))
                                  & BB_SQUARES[board.en_passant_square];
            captures |= ep_capture;
        }

        return single_push | double_push | captures;
    }
}


/**
 * @param color the color of the knight
 * @param square the square the knight is on
 * @return where the knight can move from the given square
 */
uint64_t get_knight_moves(bool color, int square) {
    uint64_t moves = BB_KNIGHT_ATTACKS[square];
    return moves & ~(board.w_occupied * color + board.b_occupied * !color);
}


/**
 * @param color the color of the bishop
 * @param square the square the bishop is on
 * @return where the bishop can move from the given square
 */
uint64_t get_bishop_moves(bool color, int square) {
    uint64_t occupied = board.occupied & BB_BISHOP_ATTACK_MASKS[square];
    uint64_t key = (occupied * BISHOP_MAGICS[square]) >> BISHOP_ATTACK_SHIFTS[square];
    uint64_t moves = BB_BISHOP_ATTACKS[square][key];
    return moves & ~(board.w_occupied * color + board.b_occupied * !color);
}


/**
 * @param color the color of the rook
 * @param square the square the rook is on
 * @return where the rook can move from the given square
 */
uint64_t get_rook_moves(bool color, int square) {
    uint64_t occupied = board.occupied & BB_ROOK_ATTACK_MASKS[square];
    uint64_t key = (occupied * ROOK_MAGICS[square]) >> ROOK_ATTACK_SHIFTS[square];
    uint64_t moves = BB_ROOK_ATTACKS[square][key];
    return moves & ~(board.w_occupied * color + board.b_occupied * !color);
}


/**
 * @param color the color of the queen
 * @param square the square the queen is on
 * @return where the queen can move from the given square
 */
uint64_t get_queen_moves(bool color, int square) {
    uint64_t bishop_occupied = board.occupied & BB_BISHOP_ATTACK_MASKS[square];
    uint64_t bishop_key = (bishop_occupied * BISHOP_MAGICS[square]) >> BISHOP_ATTACK_SHIFTS[square];
    uint64_t bishop_moves = BB_BISHOP_ATTACKS[square][bishop_key];

    uint64_t rook_occupied = board.occupied & BB_ROOK_ATTACK_MASKS[square];
    uint64_t rook_key = (rook_occupied * ROOK_MAGICS[square]) >> ROOK_ATTACK_SHIFTS[square];
    uint64_t rook_moves = BB_ROOK_ATTACKS[square][rook_key];

    uint64_t moves = bishop_moves | rook_moves;

    return moves & ~(board.w_occupied * color + board.b_occupied * !color);
}


/**
 * @param color the color of the king
 * @param square the square the king is on
 * @return where the king can move from the given square
 */
uint64_t get_king_moves(bool color, int square) {
    uint64_t moves = BB_KING_ATTACKS[square];
    if (color == WHITE) {
        if (board.w_kingside_castling_rights) set_bit(&moves, G1);
        if (board.w_queenside_castling_rights) set_bit(&moves, C1);
        return moves & ~board.w_occupied;
    } else {
        if (board.b_kingside_castling_rights) set_bit(&moves, G8);
        if (board.b_queenside_castling_rights) set_bit(&moves, C8);
        return moves & ~board.b_occupied;
    }
}


/**
 * @param knights
 * @return the attack mask of all the knights.
 */
uint64_t get_knight_mask_setwise(uint64_t knights) {
    uint64_t l1 = (knights >> 1) & 0x7f7f7f7f7f7f7f7f;
    uint64_t l2 = (knights >> 2) & 0x3f3f3f3f3f3f3f3f;
    uint64_t r1 = (knights << 1) & 0xfefefefefefefefe;
    uint64_t r2 = (knights << 2) & 0xfcfcfcfcfcfcfcfc;
    uint64_t h1 = l1 | r1;
    uint64_t h2 = l2 | r2;
    return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}


/**
 * @param color
 * @return the bitboard of all the attacks the color's pawn can make,
 * excluding en passant.
 */
uint64_t get_pawn_attacks_setwise(bool color) {
    if (color == WHITE) {
        return (((board.w_pawns << 9) & ~BB_FILE_A) | ((board.w_pawns << 7) & ~BB_FILE_H));
    } else {
        return (((board.b_pawns >> 9) & ~BB_FILE_H) | ((board.b_pawns >> 7) & ~BB_FILE_A));
    }
}


/**
 * Generates bishop slider moves using the Kogge-Stone algorithm.
 * @param bishops
 * @param empty the bitboard of space the ray can move through.
 * @return a set of bishop rays, stopping before it hits an occupied piece.
 */
uint64_t get_bishop_rays_setwise(uint64_t bishops, uint64_t empty) {
    return (_get_ray_setwise_northeast(bishops, empty) | _get_ray_setwise_northwest(bishops, empty)
            | _get_ray_setwise_southeast(bishops, empty) | _get_ray_setwise_southwest(bishops, empty));
}


/**
 * Generates rook slider moves using the Kogge-Stone algorithm.
 * @param rooks
 * @param empty the bitboard of space the ray can move through.
 * @return a set of rook rays, stopping before it hits an occupied piece.
 */
uint64_t get_rook_rays_setwise(uint64_t rooks, uint64_t empty) {
    return (_get_ray_setwise_north(rooks, empty) | _get_ray_setwise_east(rooks, empty)
            | _get_ray_setwise_south(rooks, empty) | _get_ray_setwise_west(rooks, empty));
}


/**
 * Generates queen slider moves using the Kogge-Stone algorithm.
 * @param rooks
 * @param empty the bitboard of space the ray can move through.
 * @return a set of queen rays, stopping before it hits an occupied piece.
 */
uint64_t get_queen_rays_setwise(uint64_t queens, uint64_t empty) {
    return (get_bishop_rays_setwise(queens, empty) | get_rook_rays_setwise(queens, empty));
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_south(uint64_t pieces, uint64_t empty) {
    pieces |= empty & (pieces >>  8);
    empty &= (empty >>  8);
    pieces |= empty & (pieces >> 16);
    empty &= (empty >> 16);
    pieces |= empty & (pieces >> 32);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_north(uint64_t pieces, uint64_t empty) {
    pieces |= empty & (pieces <<  8);
    empty &= (empty <<  8);
    pieces |= empty & (pieces << 16);
    empty &= (empty << 16);
    pieces |= empty & (pieces << 32);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_east(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_A;
    pieces |= empty & (pieces << 1);
    empty &= (empty << 1);
    pieces |= empty & (pieces << 2);
    empty &= (empty << 2);
    pieces |= empty & (pieces << 4);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_northeast(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_A;
    pieces |= empty & (pieces <<  9);
    empty &= (empty <<  9);
    pieces |= empty & (pieces << 18);
    empty &= (empty << 18);
    pieces |= empty & (pieces << 36);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_southeast(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_A;
    pieces |= empty & (pieces >>  7);
    empty &= (empty >>  7);
    pieces |= empty & (pieces >> 14);
    empty &= (empty >> 14);
    pieces |= empty & (pieces >> 28);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_west(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_H;
    pieces |= empty & (pieces >> 1);
    empty &= (empty >> 1);
    pieces |= empty & (pieces >> 2);
    empty &= (empty >> 2);
    pieces |= empty & (pieces >> 4);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_southwest(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_H;
    pieces |= empty & (pieces >>  9);
    empty &= (empty >>  9);
    pieces |= empty & (pieces >> 18);
    empty &= (empty >> 18);
    pieces |= empty & (pieces >> 36);
    return pieces;
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
static uint64_t _get_ray_setwise_northwest(uint64_t pieces, uint64_t empty) {
    empty &= ~BB_FILE_H;
    pieces |= empty & (pieces <<  7);
    empty &= (empty <<  7);
    pieces |= empty & (pieces << 14);
    empty &= (empty << 14);
    pieces |= empty & (pieces << 28);
    return pieces;
}
