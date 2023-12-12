#include <iostream>

#include "stack.h"
#include "movegen.h"
#include "bitboard.h"
#include "search.h"

// Pseudo-legal bitboards indexed by square to determine where that piece can attack
const uint64_t MoveGen::BB_KNIGHT_ATTACKS[64] = {
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

const uint64_t MoveGen::BB_KING_ATTACKS[64] = {
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
const uint64_t MoveGen::BISHOP_MAGICS[64] = {
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

const uint64_t MoveGen::ROOK_MAGICS[64] = {
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

void MoveGen::initMoveGenData() {
    MoveGen::init_bishop_attacks();
    MoveGen::init_rook_attacks();
    MoveGen::_init_rays();
}

/**
 * Initalizes the bishop attack magic bitboard
 * @author github.com/nkarve
 */
void MoveGen::init_bishop_attacks() {
    for (int square = Squares::A1; square <= Squares::H8; ++square) {
        uint64_t edges = ((Bitboard::BB_RANK_1 | Bitboard::BB_RANK_8) & ~Bitboard::BB_RANKS[Bitboard::rankOf(square)]) |
                         ((Bitboard::BB_FILE_A | Bitboard::BB_FILE_H) & ~Bitboard::BB_FILES[Bitboard::fileOf(square)]);
        MoveGen::BB_BISHOP_ATTACK_MASKS[square] =
                (Bitboard::BB_DIAGONALS[Bitboard::diagonalOf(square)] ^ Bitboard::BB_ANTI_DIAGONALS[Bitboard::antiDiagonalOf(square)]) & ~edges;
        uint64_t attack_mask = MoveGen::BB_BISHOP_ATTACK_MASKS[square];

        int shift = 64 - BitUtils::popCount(attack_mask);
        MoveGen::BISHOP_ATTACK_SHIFTS[square] = shift;

        uint64_t subset = 0;
        do {
            uint64_t index = subset;
            index *= BISHOP_MAGICS[square];
            index >>= shift;
            MoveGen::BB_BISHOP_ATTACKS[square][index] = _init_bishop_attacks_helper(square, subset);
            subset = (subset - attack_mask) & attack_mask;
        } while (subset);
    }
}


/**
 * Initalizes the rook attack magic bitboard
 * @author github.com/nkarve
 */
void MoveGen::init_rook_attacks() {
    for (int square = Squares::A1; square <= Squares::H8; square++) {
        uint64_t edges = ((Bitboard::BB_RANK_1 | Bitboard::BB_RANK_8) & ~Bitboard::BB_RANKS[Bitboard::rankOf(square)]) |
                         ((Bitboard::BB_FILE_A | Bitboard::BB_FILE_H) & ~Bitboard::BB_FILES[Bitboard::fileOf(square)]);
        MoveGen::BB_ROOK_ATTACK_MASKS[square] = (Bitboard::BB_RANKS[Bitboard::rankOf(square)] ^ Bitboard::BB_FILES[Bitboard::fileOf(square)]) & ~edges;
        uint64_t attack_mask = MoveGen::BB_ROOK_ATTACK_MASKS[square];

        int shift = 64 - BitUtils::popCount(attack_mask);
        MoveGen::ROOK_ATTACK_SHIFTS[square] = shift;

        uint64_t subset = 0;
        do {
            uint64_t index = subset;
            index *= MoveGen::ROOK_MAGICS[square];
            index >>= shift;
            MoveGen::BB_ROOK_ATTACKS[square][index] = _init_rook_attacks_helper(square, subset);
            subset = (subset - attack_mask) & attack_mask;
        } while (subset);
    }
}

void MoveGen::_init_rays() {
    for (int square1 = Squares::A1; square1 <= Squares::H8; square1++) {
        for (int square2 = Squares::A1; square2 <= Squares::H8; square2++) {
            if (square1 == square2) {
                Bitboard::BB_RAYS[square1][square2] = 0ULL;
                continue;
            }

            uint64_t square2_bb = Bitboard::BB_SQUARES[square2];

            uint64_t rank = Bitboard::BB_RANKS[Bitboard::rankOf(square1)];
            if (rank & square2_bb) {
                Bitboard::BB_RAYS[square1][square2] = rank;
                continue;
            }

            uint64_t file = Bitboard::BB_FILES[Bitboard::fileOf(square1)];
            if (file & square2_bb) {
                Bitboard::BB_RAYS[square1][square2] = file;
                continue;
            }

            uint64_t diagonal = Bitboard::BB_DIAGONALS[Bitboard::diagonalOf(square1)];
            if (diagonal & square2_bb) {
                Bitboard::BB_RAYS[square1][square2] = diagonal;
                continue;
            }

            uint64_t anti_diagonal = Bitboard::BB_ANTI_DIAGONALS[Bitboard::antiDiagonalOf(square1)];
            if (anti_diagonal & square2_bb) {
                Bitboard::BB_RAYS[square1][square2] = anti_diagonal;
                continue;
            }

            Bitboard::BB_RAYS[square1][square2] = 0ULL;
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
uint64_t MoveGen::_init_bishop_attacks_helper(int square, uint64_t subset) {
    uint64_t square_mask = Bitboard::BB_SQUARES[square];
    uint64_t diagonal_mask = Bitboard::BB_DIAGONALS[Bitboard::diagonalOf(square)];
    uint64_t anti_diagonal_mask = Bitboard::BB_ANTI_DIAGONALS[Bitboard::antiDiagonalOf(square)];

    uint64_t diagonal_attacks = (((diagonal_mask & subset) - square_mask * 2) ^
                                 BitUtils::_get_reverse_bb(
                                         BitUtils::_get_reverse_bb(diagonal_mask & subset) - BitUtils::_get_reverse_bb(square_mask) * 2)) &
                                diagonal_mask;

    uint64_t anti_diagonal_attacks = (((anti_diagonal_mask & subset) - square_mask * 2) ^
                                      BitUtils::_get_reverse_bb(BitUtils::_get_reverse_bb(anti_diagonal_mask & subset) -
                                                      BitUtils::_get_reverse_bb(square_mask) * 2)) &
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
uint64_t MoveGen::_init_rook_attacks_helper(int square, uint64_t subset) {
    uint64_t square_mask = Bitboard::BB_SQUARES[square];
    uint64_t rank_mask = Bitboard::BB_RANKS[Bitboard::rankOf(square)];
    uint64_t file_mask = Bitboard::BB_FILES[Bitboard::fileOf(square)];

    uint64_t rank_attacks = (((rank_mask & subset) - square_mask * 2) ^
                             BitUtils::_get_reverse_bb(BitUtils::_get_reverse_bb(rank_mask & subset) - BitUtils::_get_reverse_bb(square_mask) * 2)) &
                            rank_mask;

    uint64_t file_attacks = (((file_mask & subset) - square_mask * 2) ^
                             BitUtils::_get_reverse_bb(BitUtils::_get_reverse_bb(file_mask & subset) - BitUtils::_get_reverse_bb(square_mask) * 2)) &
                            file_mask;

    return rank_attacks | file_attacks;
}


/**
 * @param bb
 * @return the reverse of the bitboard. Flips the perspective of the board
 * @author github.com/nkarve
 */
uint64_t BitUtils::_get_reverse_bb(uint64_t bb) {
    bb = (bb & 0x5555555555555555) << 1 | (bb >> 1) & 0x5555555555555555;
    bb = (bb & 0x3333333333333333) << 2 | (bb >> 2) & 0x3333333333333333;
    bb = (bb & 0x0f0f0f0f0f0f0f0f) << 4 | (bb >> 4) & 0x0f0f0f0f0f0f0f0f;
    bb = (bb & 0x00ff00ff00ff00ff) << 8 | (bb >> 8) & 0x00ff00ff00ff00ff;
    return (bb << 48) | ((bb & 0xffff0000) << 16) | ((bb >> 16) & 0xffff0000) | (bb >> 48);
}

/**
 * @param color
 * @return the bitboard of all the attacks the color's pawn can make,
 * excluding en passant.
 */
uint64_t MoveGen::get_pawn_attacks_setwise(uint64_t pawns, bool color) {
    if (color == WHITE) {
        return (((pawns << 9) & ~Bitboard::BB_FILE_A) | ((pawns << 7) & ~Bitboard::BB_FILE_H));
    } else {
        return (((pawns >> 9) & ~Bitboard::BB_FILE_H) | ((pawns >> 7) & ~Bitboard::BB_FILE_A));
    }    
}

/**
 * @param knights
 * @return the attack mask of all the knights.
 */
uint64_t MoveGen::get_knight_mask_setwise(uint64_t knights) {
    uint64_t l1 = (knights >> 1) & 0x7f7f7f7f7f7f7f7f;
    uint64_t l2 = (knights >> 2) & 0x3f3f3f3f3f3f3f3f;
    uint64_t r1 = (knights << 1) & 0xfefefefefefefefe;
    uint64_t r2 = (knights << 2) & 0xfcfcfcfcfcfcfcfc;
    uint64_t h1 = l1 | r1;
    uint64_t h2 = l2 | r2;
    return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}


/**
 * Generates bishop slider moves using the Kogge-Stone algorithm.
 * @param bishops
 * @param empty the bitboard of space the ray can move through.
 * @return a set of bishop rays, stopping before it hits an occupied piece.
 */
uint64_t MoveGen::get_bishop_rays_setwise(uint64_t bishops, uint64_t empty) {
    return (_get_ray_setwise_northeast(bishops, empty) | _get_ray_setwise_northwest(bishops, empty)
            | _get_ray_setwise_southeast(bishops, empty) | _get_ray_setwise_southwest(bishops, empty));
}


/**
 * Generates rook slider moves using the Kogge-Stone algorithm.
 * @param rooks
 * @param empty the bitboard of space the ray can move through.
 * @return a set of rook rays, stopping before it hits an occupied piece.
 */
uint64_t MoveGen::get_rook_rays_setwise(uint64_t rooks, uint64_t empty) {
    return (_get_ray_setwise_north(rooks, empty) | _get_ray_setwise_east(rooks, empty)
            | _get_ray_setwise_south(rooks, empty) | _get_ray_setwise_west(rooks, empty));
}


/**
 * Generates queen slider moves using the Kogge-Stone algorithm.
 * @param rooks
 * @param empty the bitboard of space the ray can move through.
 * @return a set of queen rays, stopping before it hits an occupied piece.
 */
uint64_t MoveGen::get_queen_rays_setwise(uint64_t queens, uint64_t empty) {
    return (MoveGen::get_bishop_rays_setwise(queens, empty) | MoveGen::get_rook_rays_setwise(queens, empty));
}


/**
 * @param pieces
 * @param empty the bitboard of space the ray can move through.
 * @return a ray in the direction, stopping before it hits an occupied piece.
 */
uint64_t MoveGen::_get_ray_setwise_south(uint64_t pieces, uint64_t empty) {
    pieces |= empty & (pieces >> 8);
    empty &= (empty >> 8);
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
uint64_t MoveGen::_get_ray_setwise_north(uint64_t pieces, uint64_t empty) {
    pieces |= empty & (pieces << 8);
    empty &= (empty << 8);
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
uint64_t MoveGen::_get_ray_setwise_east(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_A;
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
uint64_t MoveGen::_get_ray_setwise_northeast(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_A;
    pieces |= empty & (pieces << 9);
    empty &= (empty << 9);
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
uint64_t MoveGen::_get_ray_setwise_southeast(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_A;
    pieces |= empty & (pieces >> 7);
    empty &= (empty >> 7);
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
uint64_t MoveGen::_get_ray_setwise_west(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_H;
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
uint64_t MoveGen::_get_ray_setwise_southwest(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_H;
    pieces |= empty & (pieces >> 9);
    empty &= (empty >> 9);
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
uint64_t MoveGen::_get_ray_setwise_northwest(uint64_t pieces, uint64_t empty) {
    empty &= ~Bitboard::BB_FILE_H;
    pieces |= empty & (pieces << 7);
    empty &= (empty << 7);
    pieces |= empty & (pieces << 14);
    empty &= (empty << 14);
    pieces |= empty & (pieces << 28);
    return pieces;
}
