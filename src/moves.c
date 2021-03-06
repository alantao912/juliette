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
        printf("OOM Exception!\n");
        return OOM;
    }
    memcpy(new_buff, ml->moves, sizeof(uint16_t) * ml->capacity);
    free(ml->moves);
    ml->moves = new_buff;
    ml->capacity *= 2;
    return 0;
}

static void add_move(move_list *ml, uint16_t move) {
    if (ml->size == ml->capacity) {
        resize(ml);
    }
    ml->moves[ml->size] = move;
    ++ml->size;
}

static bool can_pawn_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    if (IS_BLACK(bb->squares[SQUARE(src_file, src_rank)])) {
        return src_rank - dest_rank == 1 && abs(src_file - dest_file) == 1;
    } else {
        return src_rank - dest_rank == -1 && abs(src_file - dest_file) == 1;
    }
}

static bool can_knight_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    char dx = src_file - dest_file;
    char dy = src_rank - dest_rank;

    return (abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1);
}

static bool can_bishop_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    if (abs(src_file - dest_file) != abs(src_rank - dest_rank)) {
        /* The two points here are not on the same diagonal*/
        return false;
    }
    char dx = 1 - (2 * (src_file > dest_file));
    char dy = 1 - (2 * (src_rank > dest_rank));

    dest_file -= dx;
    dest_rank -= dy;

    while (src_file != dest_file || src_rank != dest_rank) {
        src_file += dx;
        src_rank += dy;

        if (PIECE_TYPE(bb->squares[SQUARE(src_file, src_rank)]) != EMPTY_SQUARE) {
            return false;
        }
    }
    return true;
}

static bool can_rook_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    int dx = dest_file - src_file;
    int dy = dest_rank - src_rank;
    if (dx != 0 && dy != 0) {
        /* The two points here are not on the same diagonal */
        return false;
    }

    if (dx) {
        dx /= abs(dx);
    }
    if (dy) {
        dy /= abs(dy);
    }

    dest_file -= dx;
    dest_rank -= dy;
    while (src_file != dest_file || src_rank != dest_rank) {
        src_file += dx;
        src_rank += dy;

        if (PIECE_TYPE(bb->squares[SQUARE(src_file, src_rank)]) != EMPTY_SQUARE) {
            return false;
        }
    }
    return true;
}

static bool can_queen_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    return can_bishop_attack_square(bb, src_file, src_rank, dest_file, dest_rank) || can_rook_attack_square(bb, src_file, src_rank, dest_file, dest_rank);
}

static bool can_king_attack_square(board *bb, char src_file, char src_rank, char dest_file, char dest_rank) {
    char dx = src_file - dest_file;
    char dy = src_rank - dest_rank;
    return abs(dx) <= 1 && abs(dy) <= 1;
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
            printf("UNKNOWN PIECE");
    }
    if (PIECE_TYPE(piece_data) == PAWN) {
        if (src_file != dest_file) {
            printf("x");
        }
    } else if (GET_CAPTURED_PIECE(move) != EMPTY_SQUARE) {
        printf("x");
    }
    printf("%c%d", dest_file + 'a', dest_rank);
    if (PIECE_TYPE(piece_data) == PAWN && ((IS_WHITE(piece_data) && dest_rank == 8) || (IS_BLACK(piece_data) && dest_rank == 1))) {
        printf("=%c", get_piece_rep(GET_CAPTURED_PIECE(move)));
    }
    printf("\n");
}

void make_move(board *bb, uint16_t move) {
    if (board_last_pawn_data) {
        REM_PAWN_MOVED_TWO(*board_last_pawn_data);
        board_last_pawn_data = NULL;
    }

    /* Make a local copy of piece data, all modifications of piece data go on local copy */
    char src_file = GET_SRC_FILE(move), src_rank = GET_SRC_RANK(move), src_piece_data = bb->squares[SQUARE(src_file, src_rank)];

    /* Push old piece data onto a stack */
    move_stack[curr_move_depth] = move;
    piece_data_stack[curr_move_depth] = src_piece_data;
    ++curr_move_depth;

    char dest_file = GET_DEST_FILE(move), dest_rank = GET_DEST_RANK(move);

    /* Manipulate piece data as needed */
    if (PIECE_TYPE(src_piece_data) == KING) {
        /* Castling is the only time when src_file - dest_file == 2. Thus, we can use this to check if the move is a castle move. */
        if (src_file - dest_file == 2) {
            /* King is long castling: Grab rook data from the board, and move it to the castled square */
            char rook_piece_data;
            if (bb->move == BLACK) {
                rook_piece_data = bb->squares[SQUARE(a, 8)];
                bb->squares[SQUARE(a, 8)] = EMPTY_SQUARE;
                bb->squares[SQUARE(d, 8)] = rook_piece_data;
                rook_piece_data = 8;
            } else  {
                /* King is long castling: Grab rook data from the board, and move it to the castled square */
                rook_piece_data = bb->squares[SQUARE(a, 1)];
                bb->squares[SQUARE(a, 1)] = EMPTY_SQUARE;
                bb->squares[SQUARE(d, 1)] = rook_piece_data;
                rook_piece_data = 1;
            }
            /* Update the rook's position in the piece list during long castle*/
            for (char i = 0; i < bb->num_uncaptured; ++i) {
                if (bb->pieces[i].file == a && bb->pieces[i].rank == rook_piece_data) {
                    bb->pieces[i].file = d;
                    break;
                }
            }
        } else if (src_file - dest_file == -2) {
            /* King is short castling: Grab rook data from the board, and move it to the castled square */
            char rook_piece_data;
            if (bb->move == BLACK) {
                rook_piece_data = bb->squares[SQUARE(h, 8)];
                bb->squares[SQUARE(h, 8)] = EMPTY_SQUARE;
                bb->squares[SQUARE(f, 8)] = rook_piece_data;
                rook_piece_data = 8;
            } else {
                rook_piece_data = bb->squares[SQUARE(h, 1)];
                bb->squares[SQUARE(h, 1)] = EMPTY_SQUARE;
                bb->squares[SQUARE(f, 1)] = rook_piece_data;
                rook_piece_data = 1;
            }
            /* Update the rook's position in the piece list during short castle */
            for (char i = 0; i < bb->num_uncaptured; ++i) {
                if (bb->pieces[i].file == h && bb->pieces[i].rank == rook_piece_data) {
                    bb->pieces[i].file = f;
                    break;
                }
            }
        }
        /* King's position during castling is updated elsewhere. */

        /* If a king move's at all, it loses both castling rights. */
        REM_CASTLE_LONG(src_piece_data);
        REM_CASTLE_SHORT(src_piece_data);

        /* Removes the king's castling right in the piece list. */
        if (IS_BLACK(src_piece_data)) {
            REM_CASTLE_LONG(bb->pieces[1].piece_data);
            REM_CASTLE_SHORT(bb->pieces[1].piece_data);
        } else {
            REM_CASTLE_LONG(bb->pieces[0].piece_data);
            REM_CASTLE_SHORT(bb->pieces[0].piece_data);
        }
    } else if (PIECE_TYPE(src_piece_data) == PAWN) {
        if (abs(src_rank - dest_rank) == 2) {
            /* Pawn just moved two spaces. Set the pawn's status to 'just_moved_two' */
            SET_PAWN_MOVED_TWO(src_piece_data);
            board_last_pawn_data = &bb->squares[SQUARE(dest_file, dest_rank)];
        }

        /* White pawn has reached 8th rank, or black pawn has reached 1st rank. Handle promotion. */
        if (IS_WHITE(src_piece_data) && dest_rank == 8) {
            /* Captured piece is interpreted as promoted piece */
            src_piece_data = 0 | WHITE | GET_CAPTURED_PIECE(move);
        } else if (IS_BLACK(src_piece_data) && dest_rank == 1) {
            /* Captured piece is interpreted as promoted piece */
            src_piece_data = 0 | BLACK | GET_CAPTURED_PIECE(move);
        }

        /*
            If dest_file == src_file pawn has not captured anything. Used to differentiate promotions from capture-promotions.
            If non-capture-promotion occurs, set CAPTURED_PIECE_TYPE bits in move to 000. So the following code doesn't think the pawn captured a piece.
        */

        if (dest_file == src_file) {
            move = ~(7 << 12) & move;
        }
    } else if (PIECE_TYPE(src_piece_data) == ROOK) {
        /*
            IS_ENPASSANT() flag on rook is used to denote that a certain rook move has removed the king's ability to castle on a given side.
        */
        if (bb->move == WHITE) {
            /* Remove castling rights for white */
            if (IS_QUEEN_ROOK(src_piece_data)) {
                /* Remove queen-side castling rights */
                if (CAN_CASTLE_LONG(bb->pieces[0].piece_data)) {
                    REM_CASTLE_LONG(bb->pieces[0].piece_data);
                    REM_CASTLE_LONG(bb->squares[SQUARE(bb->pieces[0].file, bb->pieces[0].rank)]);
                    move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
                }
            } else if (IS_KING_ROOK(src_piece_data)) {
                if (CAN_CASTLE_SHORT(bb->pieces[0].piece_data)) {
                    /* Remove king-side castling rights */
                    REM_CASTLE_SHORT(bb->pieces[0].piece_data);
                    REM_CASTLE_SHORT(bb->squares[SQUARE(bb->pieces[0].file, bb->pieces[0].rank)]);
                    move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
                }
            }
        } else if (IS_QUEEN_ROOK(src_piece_data)) {
            /* Remove castling rights for black */
            /* Remove queen-side castling rights */
            if (CAN_CASTLE_LONG(bb->pieces[1].piece_data)) {
                REM_CASTLE_LONG(bb->pieces[1].piece_data);
                REM_CASTLE_LONG(bb->squares[SQUARE(bb->pieces[1].file, bb->pieces[1].rank)]);
                move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
            }
        } else if (IS_KING_ROOK(src_piece_data)) {
            /* Remove king-side castling rights */
            if (CAN_CASTLE_SHORT(bb->pieces[1].piece_data)) {
                REM_CASTLE_SHORT(bb->pieces[1].piece_data);
                REM_CASTLE_SHORT(bb->squares[SQUARE(bb->pieces[1].file, bb->pieces[1].rank)]);
                move_stack[curr_move_depth - 1] = move_stack[curr_move_depth - 1] | IS_ENPASSANT();
            }
        }
    }

    if (GET_CAPTURED_PIECE(move) != EMPTY_SQUARE) {
        /* Piece was captured on this move. */
        if (!GET_IS_ENPASSANT(move) || PIECE_TYPE(src_piece_data) == ROOK) {
            /*
                Capture was not an en passant capture. Find captured piece in the piece-list, and 'remove'
                it by moving it to the back of the list and decrementing the size by one.
            */
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
            bb->squares[SQUARE(dest_file, dest_rank)] = EMPTY_SQUARE;
        } else if (PIECE_TYPE(src_piece_data) == PAWN) {

            /* When en-passant is played, the location of the pawn captured is not equal to the destination square of the attacking pawn */
            /* Rank offset corrects this */
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
            bb->squares[SQUARE(dest_file, dest_rank + rank_offset)] = EMPTY_SQUARE;
        }
    }
    /* Search through piece list to update position, and piece data */
    for (char i = 0; i < bb->num_uncaptured; ++i) {
        if (bb->pieces[i].file == src_file && bb->pieces[i].rank == src_rank) {
            bb->pieces[i].file = dest_file;
            bb->pieces[i].rank = dest_rank;
            bb->pieces[i].piece_data = src_piece_data;
            break;
        }
    }
    /* Move piece data to destination square, and clear source square */
    bb->squares[SQUARE(dest_file, dest_rank)] = src_piece_data;
    bb->squares[SQUARE(src_file, src_rank)] = EMPTY_SQUARE;

    /* Alternate move order */
    if (bb->move == BLACK) {
        bb->move = WHITE;
    } else {
        bb->move = BLACK;
    }
}

uint16_t unmake_move(board *bb) {
    --curr_move_depth;
    uint16_t prev_move = move_stack[curr_move_depth];

    char src_file = GET_SRC_FILE(prev_move), src_rank = GET_SRC_RANK(prev_move);
    char dest_file = GET_DEST_FILE(prev_move), dest_rank = GET_DEST_RANK(prev_move);

    char prev_piece_data = piece_data_stack[curr_move_depth];

    /* Move piece back to the square it came from, and then clear the square it was at. */
    bb->squares[SQUARE(src_file, src_rank)] = prev_piece_data;
    bb->squares[SQUARE(dest_file, dest_rank)] = EMPTY_SQUARE;

    switch (PIECE_TYPE(prev_piece_data)) {
        case KING: {
            char rook_piece_data, rook_og_rank = 1 + 7 * IS_BLACK(prev_piece_data), rook_og_file;
            bool is_castle = true;
            if (dest_file - src_file == -2) {

                /* Previous move was long castle */
                rook_piece_data = bb->squares[SQUARE(dest_file + 1, dest_rank)];
                bb->squares[SQUARE(dest_file + 1, dest_rank)] = EMPTY_SQUARE;

                rook_og_file = a;
                bb->squares[SQUARE(rook_og_file, rook_og_rank)] = rook_piece_data;
            } else if (dest_file - src_file == 2) {
                /* Previous move was short castle */
                rook_piece_data = bb->squares[SQUARE(dest_file - 1, dest_rank)];
                bb->squares[SQUARE(dest_file - 1, dest_rank)] = EMPTY_SQUARE;

                rook_og_file = h;
                bb->squares[SQUARE(rook_og_file, rook_og_rank)] = rook_piece_data;
            } else {
                is_castle = false;
            }

            if (is_castle) {
                char file_offset = (dest_file - src_file) / -2;
                for (char i = 0; i < bb->num_uncaptured; ++i) {
                    if (bb->pieces[i].file == dest_file + file_offset && bb->pieces[i].rank == dest_rank) {
                        bb->pieces[i].file = rook_og_file;
                        bb->pieces[i].rank = rook_og_rank;
                        bb->pieces[i].piece_data = rook_piece_data;
                        break;
                    }
                }
            }
        }
        break;
        case ROOK:
            /* IS_ENPASSANT flag for rook moves are used to denote moves that removed the king's right to castle */
            if (GET_IS_ENPASSANT(prev_move)) {
                /*If IS_BLACK() is true, returns 1 which grabs the black king. 0 if false which grabs the white king */
                piece king = bb->pieces[IS_BLACK(prev_piece_data)];
                if (IS_QUEEN_ROOK(prev_piece_data)) {
                    // Black queen side rook. Restore black's ability to castle queen side
                    SET_CASTLE_LONG(king.piece_data);
                    SET_CASTLE_LONG(bb->squares[SQUARE(king.file, king.rank)]);
                } else {
                    // Black king side rook. Restore black's ability to castle king side
                    SET_CASTLE_SHORT(king.piece_data);
                    SET_CASTLE_SHORT(bb->squares[SQUARE(king.file, king.rank)]);
                }
            }
        break;
        case PAWN:
            if (src_file == dest_file) {
                prev_move = ~(7 << 12) & prev_move;
            }
        break;
    }

    if (GET_CAPTURED_PIECE(prev_move) != EMPTY_SQUARE) {
        /* The captured piece must be restored to the destination square of the previous move. */
        char dest_rank_offset = 0;
        if (PIECE_TYPE(prev_piece_data) == PAWN && GET_IS_ENPASSANT(prev_move)) {
            /* Last move was an en passant capture. The rank of the restored piece must be given the appropriate offset. */
            dest_rank_offset = 1 + -2 * IS_WHITE(bb->squares[SQUARE(src_file, src_rank)]);
        }

        char captured_piece_data = captured_piece_stack[num_captured - 1];
        --num_captured;

        bb->squares[SQUARE(dest_file, dest_rank + dest_rank_offset)] = captured_piece_data;
        bb->pieces[bb->num_uncaptured].piece_data = captured_piece_data;
        bb->pieces[bb->num_uncaptured].file = dest_file;
        bb->pieces[bb->num_uncaptured].rank = dest_rank + dest_rank_offset;
        ++bb->num_uncaptured;
    }

    if (curr_move_depth > 0) {
        uint16_t peeked = move_stack[curr_move_depth - 1];
        char *pd = &bb->squares[SQUARE(GET_DEST_FILE(peeked), GET_DEST_RANK(peeked))];
        if (PIECE_TYPE(*pd) == PAWN && abs(GET_DEST_RANK(peeked) - GET_SRC_RANK(peeked)) == 2) {
            SET_PAWN_MOVED_TWO(*pd);
        }
    }
    /* Searches through the piece-list and restores the former piece file, rank, and piece data. */
    for (char i = 0; i < bb->num_uncaptured; ++i) {
        if (bb->pieces[i].file == dest_file && bb->pieces[i].rank == dest_rank) {
            bb->pieces[i].file = src_file;
            bb->pieces[i].rank = src_rank;
            bb->pieces[i].piece_data = prev_piece_data;
            break;
        }
    }

    /* Toggle move to opposite color */
    if (bb->move == BLACK) {
        bb->move = WHITE;
    } else {
        bb->move = BLACK;
    }
    return prev_move;
}

static bool is_king_in_check(board *bb) {
    piece king;
    if (IS_BLACK(bb->move)) {
        king = bb->pieces[0];
    } else {
        king = bb->pieces[1];
    }

    for (char i = 0; i < bb->num_uncaptured; ++i) {
        if (IS_BLACK(bb->pieces[i].piece_data) != IS_BLACK(bb->move)) {
            continue;
        }

        switch (PIECE_TYPE(bb->pieces[i].piece_data)) {
            case ROOK:
                if (can_rook_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
            case BISHOP:
                if (can_bishop_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
            case KNIGHT:
                if (can_knight_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
            case PAWN:
                if (can_pawn_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
            case KING:
                if (can_king_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
            case QUEEN:
                if (can_queen_attack_square(bb, bb->pieces[i].file, bb->pieces[i].rank, king.file, king.rank)) {
                    return true;
                }
            break;
        }
    }
    return false;
}

void remove_illegal_moves(board *bb, move_list *ml) {
    uint16_t i = 0;
    while (i < ml->size) {
        uint16_t move = ml->moves[i];
        make_move(bb, move);
        if (is_king_in_check(bb)) {
            --ml->size;
            ml->moves[i] = ml->moves[ml->size];
        } else {
            ++i;
        }
        unmake_move(bb);
    }
}

move_list *generate_moves(board *bb) {
    move_list *ml = (move_list *) malloc(sizeof(move_list));

    /* OOM Exception handling */
    if (!ml) {
        printf("OOM Exception");
        return NULL;
    }
    ml->size = 0;
    ml->capacity = 20;
    ml->moves = (short *) malloc(2 * ml->capacity);

    /* OOM Exception handling*/
    if (!ml->moves) {
        printf("OOM Exception\n");
        free(ml);
        return NULL;
    }

    for (char i = 0; i < bb->num_uncaptured; ++i) {
        char piece_data = bb->pieces[i].piece_data;
        /* If color of piece is not equal to color to move, skip the piece. */
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
        }
    }
    remove_illegal_moves(bb, ml);
    return ml;
}

void reset() {
    curr_move_depth = 0;
    num_captured = 0;
}