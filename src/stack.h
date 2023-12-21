#pragma once

#include <cstdint>

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

template <typename T> struct SearchStack
{

private:
    uint8_t size;
    T backingArray[MAX_DEPTH];

public:
    void push(T element);

    void pop();

    void clear();

    const T &top();

    size_t getSize() const;
};

template <typename T> 
void SearchStack<T>::push(T element) {
    this->backingArray[(this->size)++] = element;
}

template <typename T> 
void SearchStack<T>::pop() {
    --(this->size);
}

template <typename T> 
void SearchStack<T>::clear() {
    this->size = 0;
}

template <typename T> 
const T &SearchStack<T>::top() {
    return this->backingArray[this->size - 1];
}

template <typename T> 
size_t SearchStack<T>::getSize() const {
    return (size_t) (this->size);
}