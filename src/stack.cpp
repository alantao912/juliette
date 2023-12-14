#include <unordered_map>

#include "stack.h"
#include "util.h"
#include "bitboard.h"
#include "tables.h"


LinkedStack::LinkedStack(const Bitboard &b, const move_t &previousMove)
{
    this->board = b;
    this->previousMove = previousMove;
}