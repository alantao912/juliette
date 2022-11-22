#include <stdlib.h>

#include "stack.h"
#include "util.h"
#include "bitboard.h"

extern bitboard board;
extern Stack *stack;

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
    // rtable_add(board.zobrist);
}


/**
 * Unmakes the most recent move and updates the tables.
 */
void pop(void) {
    // Update threefold rep table
    // TODO create own repetition table, and add hash to the table
    // rtable_remove(board.zobrist);

    // Update move stack
    Stack *temp = stack;
    board = stack->board;
    stack = stack->next;
    free(temp);
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