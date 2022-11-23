#include <cstdlib>
#include <unordered_map>

#include "stack.h"
#include "util.h"
#include "bitboard.h"
#include "tables.h"

extern bitboard board;
extern Stack *stack;

extern std::unordered_map<uint64_t, RTEntry> repetition_table;

/**
 * Initalizes the stack.
 */
void init_stack() {
    _free_stack();
}


/**
 * Makes the given move and updates the tables.
 * @param move
 */
void push(move_t move) {
    // Update move stack
    Stack *node = (Stack *) malloc(sizeof(Stack));
    if (!node) {
        exit(-1);
    }
    node->board = board;
    make_move(move);
    node->next = stack;
    stack = node;

    // Update threefold rep table
    // TODO create own repetition table, and add hash to the table
    auto rt_pair = repetition_table.find(board.hash_code);
    if (rt_pair != repetition_table.end()) {
        RTEntry &rt_entry = rt_pair->second;
        ++rt_entry.num_seen;
    } else {
        repetition_table.insert(std::pair<uint64_t, RTEntry>(board.hash_code, RTEntry(1)));
    }
}


/**
 * Unmakes the most recent move and updates the tables.
 */
void pop(void) {
    printf("popping()\n");
    // Update threefold rep table
    // TODO create own repetition table, and add hash to the table
    RTEntry &rt_pair = repetition_table.find(board.hash_code)->second;
    --rt_pair.num_seen;
    if (!rt_pair.num_seen) {
        repetition_table.erase(board.hash_code);
    }
    // Update move stack
    Stack *temp = stack;
    board = stack->board;
    stack = stack->next;
    free(temp);
    printf("done pop\n");
}


/**
 * Free every element in the stack.
 */
static void _free_stack() {
    while (stack) {
        Stack *temp = stack;
        stack = stack->next;
        free(temp);
    }
}