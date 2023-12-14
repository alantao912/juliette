#pragma once

#include "bitboard.h"
#include "util.h"

/**
 * A stack of all the board positions that's been reached and
 * the moves that got to them.
 */
struct LinkedStack 
{
    Bitboard board;
    LinkedStack *next;

    move_t previousMove;

public:

    LinkedStack(const Bitboard &, const move_t &);
};