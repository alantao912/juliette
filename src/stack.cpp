#include <unordered_map>

#include "stack.h"
#include "util.h"
#include "bitboard.h"
#include "tables.h"


stack_t::stack_t(const Bitboard &b, const move_t &previousMove)
{
    this->board = b;
    this->previousMove = previousMove;
}