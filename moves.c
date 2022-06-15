#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "moves.h"
#include "piece.h"
#include "board.h"

#define MAX_DEPTH 256
/* Every time a capture is made, push captured piece's data onto this stack. Every time a capture move is restored, pop from stack to restore captured piece's piece_data */
static char captured_piece_stack[32], piece_data_stack[MAX_DEPTH], num_captured = 0;
static uint16_t move_stack[MAX_DEPTH];

static char *board_last_pawn_data;

char curr_move_depth = 0;

static int resize(move_list *ml) {
    short *new_buff = (short *) malloc(2 * sizeof(uint16_t) * ml->capacity);
    if (!new_buff) {
        return OOM;
    }
    memcpy(new_buff, ml->moves, sizeof(uint16_t) * ml->capacity);
    free(ml->moves);
    ml->moves = new_buff;
    ml->capacity *= 2;
    return 0;
}

static void add_pawn_moves(board *bb, char piece_index, move_list *ml);

static void add_knight_moves(board *bb, char piece_index, move_list *ml);

static void add_bishop_moves(board *bb, char piece_index, move_list *ml);

static void add_rook_moves(board *bb, char piece_index, move_list *ml);

static void add_king_moves(board *bb, char piece_index, move_list *ml);

static void add_queen_moves(board *bb, char piece_index, move_list *ml);

void add_move(move_list *ml, uint16_t move) {
    if (ml->size == ml->capacity) {
        resize(ml);
    }
    ml->moves[ml->size] = move;
    ++ml->size;
}

move_list *generate_moves(board *bb) {
    move_list *ml = (move_list *) malloc(sizeof(move_list));
    if (!ml) {
        return NULL;
    }
    ml->size = 0;
    ml->capacity = 20;
    ml->moves = (short *) malloc(2 * ml->capacity);

    if (!ml->moves) {
        free(ml);
        return NULL;
    }


    for (char i = 0; i < bb->num_uncaptured; ++i) {
        char piece_data = bb->pieces[i].piece_data;

        if (IS_BLACK(bb->move) != IS_BLACK(piece_data)) {
            continue;
        }

        switch(PIECE_TYPE(piece_data)) {
        case PAWN:
            add_pawn_moves(bb, i, ml);
            break;
        case KNIGHT:
            add_knight_moves(bb, i, ml);
            break;
        case BISHOP:
            add_bishop_moves(bb, i, ml);
            break; 
        case ROOK:
            add_rook_moves(bb, i, ml);
            break;
        case QUEEN:
            add_queen_moves(bb, i, ml);
            break;
        case KING:
            add_king_moves(bb, i, ml);
            break;
        default:
            printf("Encountered unknown piece!\n");
        }
    }
    return ml;
}

static void add_pawn_moves(board *bb, char piece_index, move_list *ml) {
    piece p = bb->pieces[piece_index];

    char square;
    if (IS_BLACK(p.piece_data)) {
        
        square = bb->squares[SQUARE(p.file, (p.rank - 1))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE(p.file) | DEST_RANK((p.rank - 1));

            if (p.rank - 1 == 1) {
                add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                add_move(ml, move | CAPTURED_PIECE(BISHOP));
                add_move(ml, move | CAPTURED_PIECE(ROOK));
                add_move(ml, move | CAPTURED_PIECE(QUEEN));
            } else {
                add_move(ml, move);
            }

            square = bb->squares[SQUARE(p.file, p.rank - 2)];
            if (p.rank == 7 && PIECE_TYPE(square) == EMPTY_SQUARE) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE(p.file) | DEST_RANK((p.rank - 2));
                add_move(ml, move);
            }
        }
        
        if (p.file != a) {
            square = bb->squares[SQUARE((p.file - 1), (p.rank - 1))];
            if (PIECE_TYPE(square) != EMPTY_SQUARE && IS_WHITE(square)) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank - 1));
                if (p.rank - 1 == 1) {
                    add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                    add_move(ml, move | CAPTURED_PIECE(BISHOP));
                    add_move(ml, move | CAPTURED_PIECE(ROOK));
                    add_move(ml, move | CAPTURED_PIECE(QUEEN));
                } else {
                    add_move(ml, move | CAPTURED_PIECE(PIECE_TYPE(square)));
                }
            }
        }

        if (p.file != h) {
            square = bb->squares[SQUARE(p.file + 1, p.rank - 1)];
            if (PIECE_TYPE(square) != EMPTY_SQUARE && IS_WHITE(square)) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank - 1));
                
                if (p.rank - 1 == 1) {
                    add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                    add_move(ml, move | CAPTURED_PIECE(BISHOP));
                    add_move(ml, move | CAPTURED_PIECE(ROOK));
                    add_move(ml, move | CAPTURED_PIECE(QUEEN));
                } else {
                    add_move(ml, move | CAPTURED_PIECE(PIECE_TYPE(square)));
                }
            }
        }

        // TODO: add en passant capture for black

        if (p.rank == 4) {
            if (p.file != a) {
                square = bb->squares[SQUARE((p.file - 1), p.rank)];
                if (PIECE_TYPE(square) == PAWN && IS_WHITE(square) && PAWN_MOVED_TWO(square)) {
                    uint16_t move = 0;
                    move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank - 1)) | CAPTURED_PIECE((PIECE_TYPE(square))) | IS_ENPASSANT();
                    add_move(ml, move);
                }
            }

            if (p.file != h) {
                square = bb->squares[SQUARE((p.file + 1), p.rank)];
                if (PIECE_TYPE(square) == PAWN && IS_WHITE(square) && PAWN_MOVED_TWO(square)) {
                    uint16_t move = 0;
                    move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank - 1)) | CAPTURED_PIECE((PIECE_TYPE(square))) | IS_ENPASSANT();
                    add_move(ml, move);
                }
            }
        }

    } else {
        square = bb->squares[SQUARE(p.file, p.rank + 1)];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE(p.file) | DEST_RANK((p.rank + 1));
            if (p.rank + 1 == 8) {
                add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                add_move(ml, move | CAPTURED_PIECE(BISHOP));
                add_move(ml, move | CAPTURED_PIECE(ROOK));
                add_move(ml, move | CAPTURED_PIECE(QUEEN));
            } else {
                add_move(ml, move);
            }

            square = bb->squares[SQUARE(p.file, p.rank + 2)];
            if (p.rank == 2 && PIECE_TYPE(square) == EMPTY_SQUARE) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE(p.file) | DEST_RANK((p.rank + 2));
                add_move(ml, move);
            }
        }
        if (p.file != a) {
            square = bb->squares[SQUARE((p.file - 1), (p.rank + 1))];
            if (PIECE_TYPE(square) != EMPTY_SQUARE && IS_BLACK(square)) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank + 1));
                
                if (p.rank + 1 == 8) {
                    add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                    add_move(ml, move | CAPTURED_PIECE(BISHOP));
                    add_move(ml, move | CAPTURED_PIECE(ROOK));
                    add_move(ml, move | CAPTURED_PIECE(QUEEN));
                } else {
                    add_move(ml, move | CAPTURED_PIECE(PIECE_TYPE(square)));
                }
            }
        }

        if (p.file != h) {
            square = bb->squares[SQUARE(p.file + 1, p.rank + 1)];
            if (PIECE_TYPE(square) != EMPTY_SQUARE && IS_BLACK(square)) {
                uint16_t move = 0;
                move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank + 1));
                if (p.rank + 1 == 8) {
                    add_move(ml, move | CAPTURED_PIECE(KNIGHT));
                    add_move(ml, move | CAPTURED_PIECE(BISHOP));
                    add_move(ml, move | CAPTURED_PIECE(ROOK));
                    add_move(ml, move | CAPTURED_PIECE(QUEEN));
                } else {
                    add_move(ml, move | CAPTURED_PIECE(PIECE_TYPE(square)));
                }
            }
        }

        // en passant rule for white
        if (p.rank == 5) {
            if (p.file != a) {
                square = bb->squares[SQUARE(p.file - 1, p.rank)];
                if (PIECE_TYPE(square) == PAWN && IS_BLACK(square) && PAWN_MOVED_TWO(square)) {
                    uint16_t move = 0;
                    move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank + 1)) | CAPTURED_PIECE((PIECE_TYPE(square))) | IS_ENPASSANT();
                    add_move(ml, move);
                }
            }

            if (p.file != h) {
                square = bb->squares[SQUARE(p.file + 1, p.rank)];

                if (PIECE_TYPE(square) == PAWN && IS_BLACK(square) && PAWN_MOVED_TWO(square)) {
                    uint16_t move = 0;
                    move = move | SRC_FILE(p.file) | SRC_RANK(p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank + 1)) | CAPTURED_PIECE((PIECE_TYPE(square))) | IS_ENPASSANT();
                    add_move(ml, move);
                }
            }
        }
    }
}

static void add_knight_moves(board *bb, char piece_index, move_list *ml) {
    piece p = bb->pieces[piece_index];
    char square;


    if (p.rank + 2 <= 8 && p.file + 1 <= h) {
        square = bb->squares[SQUARE((p.file + 1), (p.rank + 2))];
        
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank + 2));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank + 2)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank + 1 <= 8 && p.file + 2 <= h) {
        square = bb->squares[SQUARE((p.file + 2), (p.rank + 1))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 2)) | DEST_RANK((p.rank + 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 2)) | DEST_RANK((p.rank + 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank - 1 >= 1 && p.file + 2 <= h) {
        square = bb->squares[SQUARE((p.file +  2), (p.rank - 1))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 2)) | DEST_RANK((p.rank - 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 2)) | DEST_RANK((p.rank - 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank - 2 >= 1 && p.file + 1 <= h) {
        square = bb->squares[SQUARE((p.file + 1), (p.rank - 2))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank - 2));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file + 1)) | DEST_RANK((p.rank - 2)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank - 2 >= 1 && p.file - 1 >= a) {
        square = bb->squares[SQUARE((p.file - 1), (p.rank - 2))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank - 2));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank - 2)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank - 1 >= 1 && p.file - 2 >= a) {
        square = bb->squares[SQUARE((p.file - 2), (p.rank - 1))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 2)) | DEST_RANK((p.rank - 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 2)) | DEST_RANK((p.rank - 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank + 1 <= 8 && p.file - 2 >= a) {
        square = bb->squares[SQUARE((p.file - 2), (p.rank + 1))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 2)) | DEST_RANK((p.rank + 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 2)) | DEST_RANK((p.rank + 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (p.rank + 2 <= 8 && p.file - 1 >= a) {
        square = bb->squares[SQUARE((p.file - 1), (p.rank + 2))];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank + 2));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(p.piece_data)) {
            uint16_t move = 0;
            move = move | SRC_FILE(p.file) | SRC_RANK (p.rank) | DEST_FILE((p.file - 1)) | DEST_RANK((p.rank + 2)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }
}

static void add_bishop_moves(board *bb, char piece_index, move_list *ml) {
    piece bishop = bb->pieces[piece_index];

    char i = bishop.rank, j = bishop.file, square;
    uint16_t move;

    while (i < 8 && j > a) {
        ++i;
        --j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(bishop.piece_data)) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = bishop.rank;
    j = bishop.file;
    
    while (i > 1 && j > a) {
        --i;
        --j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(bishop.piece_data)) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = bishop.rank;
    j = bishop.file;

    while (i < 8 && j < h) {
        ++i;
        ++j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(bishop.piece_data)) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = bishop.rank;
    j = bishop.file;

    while (i > 1 && j < h) {
        --i;
        ++j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(bishop.piece_data)) {
            move = move | SRC_FILE(bishop.file) | SRC_RANK(bishop.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }
}

static void add_rook_moves(board *bb, char piece_index, move_list *ml) {
    piece rook = bb->pieces[piece_index];

    char i = rook.file, square;
    uint16_t move;

    while (i < h) {
        ++i;
        move = 0;
        square = bb->squares[SQUARE(i, rook.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(i) | DEST_RANK(rook.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(rook.piece_data)) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(i) | DEST_RANK(rook.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = rook.file;

    while(i > a) {
        --i;
        move = 0;
        square = bb->squares[SQUARE(i, rook.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(i) | DEST_RANK(rook.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(rook.piece_data)) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(i) | DEST_RANK(rook.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = rook.rank;

    while (i < 8) {
        ++i;
        move = 0;
        square = bb->squares[SQUARE(rook.file, i)];

        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(rook.file) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(rook.piece_data)) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(rook.file) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = rook.rank;

    while (i > 1) {
        --i;
        move = 0;
        square = bb->squares[SQUARE(rook.file, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(rook.file) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(rook.piece_data)) {
            move = move | SRC_FILE(rook.file) | SRC_RANK(rook.rank) | DEST_FILE(rook.file) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }
}

static void add_king_moves(board *bb, char piece_index, move_list *ml) {
    piece king = bb->pieces[piece_index];

    char square;
    uint16_t move;

    if (IS_BLACK(king.piece_data)) {
        if (CAN_CASTLE_SHORT(king.piece_data) 
        // Checks if g8, f8 are empty
        && PIECE_TYPE(bb->squares[SQUARE(g, 8)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(f, 8)]) == EMPTY_SQUARE
        // Checks if the piece on h8 is a black rook
        && PIECE_TYPE(bb->squares[SQUARE(h, 8)]) == ROOK && IS_BLACK(bb->squares[SQUARE(h, 8)]) && IS_KING_ROOK(bb->squares[SQUARE(h, 8)])) {
            move = 0;
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 2)) | DEST_RANK(king.rank);
            add_move(ml, move);
        }

        if (CAN_CASTLE_LONG(king.piece_data)
        // Checks if squares d8, c8, b8 are empty
        && PIECE_TYPE(bb->squares[SQUARE(d, 8)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(c, 8)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(b, 8)]) == EMPTY_SQUARE
        // Checks if the piece on a8 is a black rook
        && PIECE_TYPE(bb->squares[SQUARE(a, 8)]) == ROOK && IS_BLACK(bb->squares[SQUARE(a, 8)]) && IS_QUEEN_ROOK(bb->squares[SQUARE(a, 8)])) {
            move = 0;
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 2)) | DEST_RANK(king.rank);
            add_move(ml, move); 
        }

    } else {

        if (CAN_CASTLE_SHORT(king.piece_data) 
        // Checks if g1, f1 are empty squares
        && PIECE_TYPE(bb->squares[SQUARE(g, 1)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(f, 1)]) == EMPTY_SQUARE 
        // Checks if h1 is a white rook
        && PIECE_TYPE(bb->squares[SQUARE(h, 1)]) == ROOK && IS_WHITE(bb->squares[SQUARE(h,1)]) && IS_KING_ROOK(bb->squares[SQUARE(h, 1)])) {
            move = 0;
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 2)) | DEST_RANK(king.rank);
            add_move(ml, move);
        }

        if (CAN_CASTLE_LONG(king.piece_data)
        // Checks if b1, c1, d1 squares are empty
        && PIECE_TYPE(bb->squares[SQUARE(d, 1)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(c, 1)]) == EMPTY_SQUARE && PIECE_TYPE(bb->squares[SQUARE(b, 1)]) == EMPTY_SQUARE
        // Checks if square a1 is a white rook
        && PIECE_TYPE(bb->squares[SQUARE(a, 1)]) == ROOK && IS_WHITE(bb->squares[SQUARE(a,1)]) && IS_QUEEN_ROOK(bb->squares[SQUARE(a, 1)])) {
            move = 0;
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 2)) | DEST_RANK(king.rank);
            add_move(ml, move);
        }
    }

    if (king.rank < 8) {
        // king can move up
        move = 0;
        square = bb->squares[SQUARE(king.file, (king.rank + 1))];
        
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE(king.file) | DEST_RANK((king.rank + 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE(king.file) | DEST_RANK((king.rank + 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }

        if (king.file > a) {
            // king can move left
            move = 0;
            square = bb->squares[SQUARE((king.file - 1), (king.rank + 1))];
            if (PIECE_TYPE(square) == EMPTY_SQUARE) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK((king.rank + 1));
                add_move(ml, move);
            } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK((king.rank + 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
                add_move(ml, move);
            }
        }

        if (king.file < h) {
            // king can move right
            move = 0;
            square = bb->squares[SQUARE((king.file + 1), (king.rank + 1))];
            if (PIECE_TYPE(square) == EMPTY_SQUARE) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK((king.rank + 1));
                add_move(ml, move);
            } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK((king.rank + 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
                add_move(ml, move);
            }
        }
    }

    if (king.file > a) {
        move = 0;
        square = bb->squares[SQUARE((king.file - 1), king.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK(king.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK(king.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (king.file < h) {
        move = 0;
        square = bb->squares[SQUARE((king.file + 1), king.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK(king.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK(king.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }
    }

    if (king.rank > 1) {
        // king can move down
        move = 0;
        square = bb->squares[SQUARE(king.file, (king.rank - 1))];
        
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE(king.file) | DEST_RANK((king.rank - 1));
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
            move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE(king.file) | DEST_RANK((king.rank - 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
        }

        if (king.file > a) {
            // king can move left
            move = 0;
            square = bb->squares[SQUARE((king.file - 1), (king.rank - 1))];
            if (PIECE_TYPE(square) == EMPTY_SQUARE) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK((king.rank - 1));
                add_move(ml, move);
            } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file - 1)) | DEST_RANK((king.rank - 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
                add_move(ml, move);
            }
        }

        if (king.file < h) {
            // king can move right
            move = 0;
            square = bb->squares[SQUARE((king.file + 1), (king.rank - 1))];
            if (PIECE_TYPE(square) == EMPTY_SQUARE) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK((king.rank - 1));
                add_move(ml, move);
            } else if (IS_BLACK(square) != IS_BLACK(king.piece_data)) {
                move = move | SRC_FILE(king.file) | SRC_RANK(king.rank) | DEST_FILE((king.file + 1)) | DEST_RANK((king.rank - 1)) | CAPTURED_PIECE(PIECE_TYPE(square));
                add_move(ml, move);
            }
        }
    }
}

static void add_queen_moves(board *bb, char piece_index, move_list *ml) {
    piece queen = bb->pieces[piece_index];

    char i = queen.rank, j = queen.file, square;
    uint16_t move;

    while (i < 8 && j > a) {
        ++i;
        --j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.rank;
    j = queen.file;
    
    while (i > 1 && j > a) {
        --i;
        --j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.rank;
    j = queen.file;

    while (i < 8 && j < h) {
        ++i;
        ++j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.rank;
    j = queen.file;

    while (i > 1 && j < h) {
        --i;
        ++j;
        move = 0;
        square = bb->squares[SQUARE(j, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(j) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    // ROOK PART
    i = queen.file;

    while (i < h) {
        ++i;
        move = 0;
        square = bb->squares[SQUARE(i, queen.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(i) | DEST_RANK(queen.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(i) | DEST_RANK(queen.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.file;

    while(i > a) {
        --i;
        move = 0;
        square = bb->squares[SQUARE(i, queen.rank)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(i) | DEST_RANK(queen.rank);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(i) | DEST_RANK(queen.rank) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.rank;

    while (i < 8) {
        ++i;
        move = 0;
        square = bb->squares[SQUARE(queen.file, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(queen.file) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(queen.file) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }

    i = queen.rank;

    while (i > 1) {
        --i;
        move = 0;
        square = bb->squares[SQUARE(queen.file, i)];
        if (PIECE_TYPE(square) == EMPTY_SQUARE) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(queen.file) | DEST_RANK(i);
            add_move(ml, move);
        } else if (IS_BLACK(square) != IS_BLACK(queen.piece_data)) {
            move = move | SRC_FILE(queen.file) | SRC_RANK(queen.rank) | DEST_FILE(queen.file) | DEST_RANK(i) | CAPTURED_PIECE(PIECE_TYPE(square));
            add_move(ml, move);
            break;
        } else {
            break;
        }
    }
}

void print_move(board *bb, uint16_t move) {

    char src_file = GET_SRC_FILE(move);
    char src_rank = GET_SRC_RANK(move);
    char dest_file = GET_DEST_FILE(move);
    char dest_rank = GET_DEST_RANK(move);

    char piece_data = bb->squares[SQUARE(src_file, src_rank)];
    switch (PIECE_TYPE(piece_data)) {
        case PAWN:
            if (GET_CAPTURED_PIECE(move) != EMPTY_SQUARE) {
                printf("%c", src_file + 'a');
            }
        break;

        case KNIGHT:
            printf("N");
        break;

        case BISHOP:
            printf("B");
        break;

        case ROOK:
            printf("R");
        break;

        case KING:
            if (src_file - dest_file == 2) {
                printf("O-O-O\n");
                return;
            } else if (src_file - dest_file == -2) {
                printf("O-O\n");
                return;
            } else {
                printf("K");
            }
        break;

        case QUEEN:
            printf("Q");
        break;
        default:
            printf("%d", SQUARE(src_file, src_rank));
    }

    if (GET_CAPTURED_PIECE(move) != EMPTY_SQUARE) {
        printf("x");
    }
    printf("%c%c", dest_file + 'a', dest_rank + '0');

    if (PIECE_TYPE(piece_data) == PAWN && ((IS_WHITE(piece_data) && dest_rank == 8) || (IS_BLACK(piece_data) && dest_rank == 1))) {
        printf("=%c", get_piece_rep(GET_CAPTURED_PIECE(move)));
    }
    printf("\n");
}

static void remove_castling(board *bb, char color, char direction) {
    for (char i = 0; i < bb->num_uncaptured; ++i) {
        if (PIECE_TYPE(bb->pieces[i].piece_data) == KING && IS_BLACK(bb->pieces[i].piece_data) == IS_BLACK(color)) {
            if (direction == 0) {
                REM_CASTLE_SHORT(bb->pieces[i].piece_data);
            } else {
                REM_CASTLE_LONG(bb->pieces[i].piece_data);
            }
            bb->squares[SQUARE(bb->pieces[i].file, bb->pieces[i].rank)] = bb->pieces[i].piece_data;
            break;
        }
    }
}

void make_move(board *bb, uint16_t move) {
    
    /* Make a local copy of piece data, all modifications of piece data go on local copy */
    char src_file = GET_SRC_FILE(move), src_rank = GET_SRC_RANK(move), src_piece_data = bb->squares[SQUARE(src_file, src_rank)];

    /* Push old piece data onto a stack */
    move_stack[curr_move_depth] = move;
    piece_data_stack[curr_move_depth] = src_piece_data;
    ++curr_move_depth;


    char dest_file = GET_DEST_FILE(move), dest_rank = GET_DEST_RANK(move);
    
    /* Manipulate piece data as needed */
    if (PIECE_TYPE(src_piece_data) == KING) {
        if (src_file - dest_file == 2) {
            /* King is long castling: Grab rook data from the board, and move it to the castled square */
            char rook_piece_data;
            if (bb->move == BLACK) {
                rook_piece_data = bb->squares[SQUARE(a, 8)];
                bb->squares[SQUARE(a, 8)] = EMPTY_SQUARE;
                bb->squares[SQUARE(d, 8)] = rook_piece_data;

                /* Update piece information in the pieces list */

                for (char i = 0; i < bb->num_uncaptured; ++i) {
                    if (bb->pieces[i].file == a && bb->pieces[i].rank == 8) {
                        bb->pieces[i].file = d;
                        break;
                    }
                }
            } else  {
                /* King is long castling: Grab rook data from the board, and move it to the castled square */
                rook_piece_data = bb->squares[SQUARE(a, 1)];
                bb->squares[SQUARE(a, 1)] = EMPTY_SQUARE;
                bb->squares[SQUARE(d, 1)] = rook_piece_data;

                /* Update piece information in the pieces list */
                for (char i = 0; i < bb->num_uncaptured; ++i) {
                    if (bb->pieces[i].file == a && bb->pieces[i].rank == 1) {
                        bb->pieces[i].file = d;
                        break;
                    }
                }
            }
        } else if (src_file - dest_file == -2) {
            /* King is short castling: Grab rook data from the board, and move it to the castled square */
            char rook_piece_data;
            if (bb->move == BLACK) {
                rook_piece_data = bb->squares[SQUARE(h, 8)];
                bb->squares[SQUARE(h, 8)] = EMPTY_SQUARE;
                bb->squares[SQUARE(f, 8)] = rook_piece_data;

                for (char i = 0; i < bb->num_uncaptured; ++i) {
                    if (bb->pieces[i].file == h && bb->pieces[i].rank == 8) {
                        bb->pieces[i].file = f;
                        break;
                    }
                }                
            } else {
                rook_piece_data = bb->squares[SQUARE(h, 1)];
                bb->squares[SQUARE(h, 1)] = EMPTY_SQUARE;
                bb->squares[SQUARE(f, 1)] = rook_piece_data;

                for (char i = 0; i < bb->num_uncaptured; ++i) {
                    if (bb->pieces[i].file == h && bb->pieces[i].rank == 1) {
                        bb->pieces[i].file = f;
                        break;
                    }
                }
            }
        }   
        REM_CASTLE_LONG(src_piece_data);
        REM_CASTLE_SHORT(src_piece_data);
    } else if (PIECE_TYPE(src_piece_data) == PAWN) {
        if (abs(src_rank - dest_rank) == 2) {
            // pawn just moved two
            SET_PAWN_MOVED_TWO(src_piece_data);
            board_last_pawn_data = &bb->squares[SQUARE(dest_file, dest_rank)];
        }

        if (IS_WHITE(src_piece_data) && dest_rank == 8) {
            src_piece_data = 0 | WHITE | GET_CAPTURED_PIECE(move);
        } else if (IS_BLACK(src_piece_data) && dest_rank == 1) {
            // captured piece is interpreted as promotion to
            src_piece_data = 0 | BLACK | GET_CAPTURED_PIECE(move);
        }
    } else if (PIECE_TYPE(src_piece_data) == ROOK) {
        if (bb->move == WHITE) {
            if (src_file == a && src_rank == 1) {

                // remove long castling for white
                if (CAN_CASTLE_LONG(bb->pieces[0].piece_data)) {
                    REM_CASTLE_LONG(bb->pieces[0].piece_data);

                    // IS_ENPASSANT() bit is used to denote that this rook move, removed the king's right to castle
                    move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
                }
                
            } else if (src_file == h && src_rank == 1) {
                // remove castle short for white
                if (CAN_CASTLE_SHORT(bb->pieces[0].piece_data)) {
                    REM_CASTLE_SHORT(bb->pieces[0].piece_data);

                     // IS_ENPASSANT() bit is used to denote that this rook move, removed the king's right to castle
                    move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
                }
            }
        } else if (src_file == a && src_rank == 8) {
            // remove castle long for black
            if (CAN_CASTLE_LONG(bb->pieces[1].piece_data)) {
                REM_CASTLE_LONG(bb->pieces[1].piece_data);

                 // IS_ENPASSANT() bit is used to denote that this rook move, removed the king's right to castle
                move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
            }
        } else if (src_file == h && src_rank == 8) {
            // remove castle short for black
            if (CAN_CASTLE_SHORT(bb->pieces[1].piece_data)) {
                REM_CASTLE_SHORT(bb->pieces[1].piece_data);

                 // IS_ENPASSANT() bit is used to denote that this rook move, removed the king's right to castle
                move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
            }
                
        }
    }
    
    if (board_last_pawn_data) {
        REM_PAWN_MOVED_TWO(*board_last_pawn_data);
    }

    if (GET_CAPTURED_PIECE(move) == EMPTY_SQUARE) {
        // No piece was captured during move, only search through piece list to find original piece, and update its location.
        for (char i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].file == src_file && bb->pieces[i].rank == src_rank) {
                bb->pieces[i].file = dest_file;
                bb->pieces[i].rank = dest_rank;
                bb->pieces[i].piece_data = src_piece_data;
                break;
            }
        }
    } else if (GET_IS_ENPASSANT(move)) {
        
        char rank_offset = 1 + (IS_WHITE(src_piece_data) * -2);

        for (char i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].rank == dest_rank + rank_offset && bb->pieces[i].file == dest_file) {
                /* Remove captured piece from the pieces list */
                piece captured_piece = bb->pieces[i];
                bb->pieces[i] = bb->pieces[bb->num_uncaptured - 1];

                --bb->num_uncaptured;
                /* Push captured piece onto the captured pieces stack */
                captured_piece_stack[num_captured] = captured_piece.piece_data;
                ++num_captured;
                break;
            }
        }

        for (char i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].rank == src_rank && bb->pieces[i].file == src_file) {
                bb->pieces[i].file = dest_file;
                bb->pieces[i].rank = dest_rank;
                bb->pieces[i].piece_data = src_piece_data;
                break;
            }
        }

        bb->squares[SQUARE(dest_file, dest_rank + rank_offset)] = EMPTY_SQUARE;
    } else {
        // Piece was captured during the move, search through piece list to find original piece, update its location. And push the captured piece onto
        // captured piece data stack.
        // Remove captured piece from the board

        for (char i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].rank == dest_rank && bb->pieces[i].file == dest_file) {
                /* Remove captured piece from the pieces list */
                piece captured_piece = bb->pieces[i];
                bb->pieces[i] = bb->pieces[bb->num_uncaptured - 1];

                --bb->num_uncaptured;
                /* Push captured piece onto the captured pieces stack */
                captured_piece_stack[num_captured] = captured_piece.piece_data;
                ++num_captured;
                break;
            }
        }

        for (char i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].rank == src_rank && bb->pieces[i].file == src_file) {
                bb->pieces[i].file = dest_file;
                bb->pieces[i].rank = dest_rank;
                bb->pieces[i].piece_data = src_piece_data;
                break;
            }
        }
        bb->squares[SQUARE(dest_file, dest_rank)] = EMPTY_SQUARE;
    }
    /* Store piece data onto the new square the piece is on after the move */
    bb->squares[SQUARE(dest_file, dest_rank)] = src_piece_data;

    /* The square that the piece was previously on is now empty. */
    bb->squares[SQUARE(src_file, src_rank)] = EMPTY_SQUARE;
    
    if (bb->move == BLACK) {
        bb->move = WHITE;
    } else {
        bb->move = BLACK;
    }
}

void unmake_move(board *bb) {
    
}