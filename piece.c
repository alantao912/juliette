#include <stdio.h>
#include "piece.h"
#include "board.h"

char get_piece_char(board *bb, char file, char rank) {
    char square = bb->squares[SQUARE(file, rank)];
    if (square == EMPTY_SQUARE) {
        return ((file + rank) % 2 == 0) * 32 + ((file + rank) % 2 != 0) * 42;
    }
    char piece_char;
    switch (PIECE_TYPE(square)) {
        case PAWN:
            piece_char = 'P';
            break;
        case KNIGHT:
            piece_char = 'N';
            break;
        case BISHOP:
            piece_char = 'B';
            break;
        case ROOK:
            piece_char = 'R';
            break;
        case QUEEN:
            piece_char = 'Q';
            break;
        case KING:
            piece_char = 'K';
            break;
    }
    
    if (IS_BLACK(square)) {
        piece_char = piece_char | 32;
    }
    return piece_char;
}

char get_piece_rep(char piece) {
    switch (piece) {
        case PAWN:
        return 'P';
        case KNIGHT:
        return 'N';
        case BISHOP:
        return 'B';
        case ROOK:
        return 'R';
        case QUEEN:
        return 'Q';
        case KING:
        return 'K';
        default:
        return ' ';
    }
}