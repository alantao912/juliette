#pragma once

#include "bitboard.h"
#include "util.h"

/**
 * A stack of all the board positions that's been reached and
 * the moves that got to them.
 */
struct stack_t 
{
    Bitboard board;
    stack_t *next;

    move_t previousMove;

public:

    stack_t(const Bitboard &, const move_t &);
};