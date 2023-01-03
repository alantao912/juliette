#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "util.h"


extern const uint64_t BB_KNIGHT_ATTACKS[64];
extern uint64_t BB_BISHOP_ATTACKS[64][512];
extern uint64_t BB_ROOK_ATTACKS[64][4096];
extern const uint64_t BB_KING_ATTACKS[64];

extern const uint64_t BISHOP_MAGICS[64];
extern const uint64_t ROOK_MAGICS[64];
extern uint64_t BB_BISHOP_ATTACK_MASKS[64];
extern uint64_t BB_ROOK_ATTACK_MASKS[64];
extern uint64_t ROOK_ATTACK_SHIFTS[64];
extern uint64_t BISHOP_ATTACK_SHIFTS[64];


void init_bishop_attacks();
void init_rook_attacks();
void _init_rays();
static uint64_t _init_bishop_attacks_helper(int square, uint64_t subset);
static uint64_t _init_rook_attacks_helper(int square, uint64_t subset);
static uint64_t _get_reverse_bb(uint64_t bb);

int gen_legal_moves(move_t* moves, bool color);
int gen_legal_captures(move_t* moves, bool color);
int gen_legal_captures_sq(move_t *moves, bool color, uint64_t square);
int gen_nonquiescent_moves(move_t *moves, bool color);

int get_flag(char piece, int from, int to);

static uint64_t _get_attackmask(bool color);
static uint64_t _get_checkmask(bool color);
static uint64_t _get_pinmask(bool color, int square);

uint64_t get_pawn_moves(bool color, int square);
uint64_t get_knight_moves(bool color, int square);
uint64_t get_bishop_moves(bool color, int square);
uint64_t get_rook_moves(bool color, int square);
uint64_t get_queen_moves(bool color, int square);
uint64_t get_king_moves(bool color, int square);

uint64_t get_pawn_attacks_setwise(bool color);
uint64_t get_knight_mask_setwise(uint64_t knights);
uint64_t get_bishop_rays_setwise(uint64_t rooks, uint64_t empty);
uint64_t get_rook_rays_setwise(uint64_t bishops, uint64_t empty);
uint64_t get_queen_rays_setwise(uint64_t queens, uint64_t empty);

static uint64_t _get_ray_setwise_south(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_north(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_east(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_northeast(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_southeast(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_west(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_southwest(uint64_t pieces, uint64_t empty);
static uint64_t _get_ray_setwise_northwest(uint64_t pieces, uint64_t empty);


#endif
