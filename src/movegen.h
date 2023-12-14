#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "util.h"

struct MoveGen 
{
    friend struct Bitboard;

public:

    static const uint64_t BB_KNIGHT_ATTACKS[64];

    static uint64_t BB_BISHOP_ATTACKS[64][512];

    static uint64_t BB_ROOK_ATTACKS[64][4096];

    static const uint64_t BB_KING_ATTACKS[64];

    static const uint64_t BISHOP_MAGICS[64];

    static const uint64_t ROOK_MAGICS[64];

    static void initMoveGenData();

    static uint64_t get_pawn_attacks_setwise(uint64_t, bool);

    static uint64_t get_knight_mask_setwise(uint64_t);

    static uint64_t get_bishop_rays_setwise(uint64_t, uint64_t);

    static uint64_t get_rook_rays_setwise(uint64_t, uint64_t);

    static uint64_t get_queen_rays_setwise(uint64_t, uint64_t);

private:

    // Attack masks and shifts for magic bitboard move generation
    static uint64_t BB_BISHOP_ATTACK_MASKS[64];
    static uint64_t BB_ROOK_ATTACK_MASKS[64];
    static uint64_t ROOK_ATTACK_SHIFTS[64];
    static uint64_t BISHOP_ATTACK_SHIFTS[64];

    static void init_bishop_attacks();

    static void init_rook_attacks();

    static void _init_rays();

    static uint64_t _init_bishop_attacks_helper(int, uint64_t);

    static uint64_t _init_rook_attacks_helper(int, uint64_t);

    static uint64_t _get_ray_setwise_south(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_north(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_east(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_northeast(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_southeast(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_west(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_southwest(uint64_t, uint64_t);

    static uint64_t _get_ray_setwise_northwest(uint64_t, uint64_t);

};


#endif
