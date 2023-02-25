#include <unordered_map>

#include "stack.h"
#include "util.h"
#include "bitboard.h"
#include "tables.h"

extern bitboard board;
extern stack_t *stack;

extern std::unordered_map<uint64_t, RTEntry> repetition_table;
extern int16_t ply;

/**
 * Initalizes the stack.
 */
void init_stack() {
    _free_stack();
    stack = nullptr;
}


/**
 * Makes the given move and updates the tables.
 * @param move
 */
void push(move_t move) {
    // Update move stack
    stack_t *node = new stack_t;

    node->board = board;
    make_move(move);
    node->next = stack;
    node->prev_mv = move;
    stack = node;

    auto rt_pair = repetition_table.find(board.hash_code);
    if (rt_pair != repetition_table.end()) {
        RTEntry &rt_entry = rt_pair->second;
        ++rt_entry.num_seen;
    } else {
        repetition_table.insert(std::pair<uint64_t, RTEntry>(board.hash_code, RTEntry(1)));
    }
    ++ply;
}


/**
 * Unmakes the most recent move and updates the tables.
 */
void pop() {
    RTEntry &rt_pair = repetition_table.find(board.hash_code)->second;
    --rt_pair.num_seen;
    /** TODO: Is this check necessary? Experiment with removal of this part*/
    if (!rt_pair.num_seen) {
        repetition_table.erase(board.hash_code);
    }
    // Update move stack
    stack_t *temp = stack;
    board = stack->board;
    stack = stack->next;
    --ply;
    delete temp;
}


/**
 * Free every element in the stack.
 */
static void _free_stack() {
    while (stack) {
        stack_t *temp = stack;
        stack = stack->next;
        delete temp;
    }
}