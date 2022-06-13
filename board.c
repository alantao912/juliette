#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"

#include "piece.h"

static void print_rank(board *bb, char rank);

board *get_starting_position() {
    board *bb = (board *) malloc(sizeof(board));
    if (!bb) {
        return NULL;
    }
    // ranks: [1, 8]
    // file : [0, 7]
    memset(&bb->squares, 0, 64);
    memset(&bb->pieces, 0, 32 * sizeof(piece));

    bb->squares[SQUARE(a, 1)] = ROOK | WHITE;
    bb->pieces[0].piece_data = ROOK | WHITE;
    bb->pieces[0].file = a;
    bb->pieces[0].rank = 1;

    bb->squares[SQUARE(b,1)] = KNIGHT | WHITE;
    bb->pieces[1].piece_data = KNIGHT | WHITE;
    bb->pieces[1].file = b;
    bb->pieces[1].rank = 1;


    bb->squares[SQUARE(c, 1)] = BISHOP | WHITE;
    bb->pieces[2].piece_data = BISHOP | WHITE;
    bb->pieces[2].file = c;
    bb->pieces[2].rank = 1;

    bb->squares[SQUARE(d, 1)] = QUEEN | WHITE;
    bb->pieces[3].piece_data = QUEEN | WHITE;
    bb->pieces[3].file = d;
    bb->pieces[3].rank = 1;

    bb->squares[SQUARE(e, 1)] = KING | WHITE;
    SET_CASTLE_SHORT(bb->squares[SQUARE(e, 1)]);
    SET_CASTLE_LONG(bb->squares[SQUARE(e, 1)]);

    bb->pieces[4].piece_data = bb->squares[SQUARE(e, 1)];
    bb->pieces[4].file = e;
    bb->pieces[4].rank = 1;

    bb->squares[SQUARE(f, 1)] = BISHOP | WHITE;
    bb->pieces[5].piece_data = BISHOP | WHITE;
    bb->pieces[5].file = f;
    bb->pieces[5].rank = 1;

    bb->squares[SQUARE(g, 1)] = KNIGHT | WHITE;
    bb->pieces[6].piece_data = KNIGHT | WHITE;
    bb->pieces[6].file = g;
    bb->pieces[6].rank = 1;
    
    bb->squares[SQUARE(h, 1)] = ROOK | WHITE;
    bb->pieces[7].piece_data = ROOK | WHITE;
    bb->pieces[7].file = h;
    bb->pieces[7].rank = 1;

    for (unsigned char i = 0; i < 8; ++i) {
        bb->squares[SQUARE(i, 2)] = PAWN | WHITE;
        bb->pieces[8 + i].piece_data = PAWN | WHITE;
        bb->pieces[8 + i].file = i;
        bb->pieces[8 + i].rank = 2;
    }

    bb->squares[SQUARE(a, 8)] = ROOK | BLACK;
    bb->pieces[16].piece_data = ROOK | BLACK;
    bb->pieces[16].file = a;
    bb->pieces[16].rank = 8;

    bb->squares[SQUARE(b,8)] = KNIGHT | BLACK;
    bb->pieces[17].piece_data = KNIGHT | BLACK;
    bb->pieces[17].file = b; 
    bb->pieces[17].rank = 8;
    
    bb->squares[SQUARE(c, 8)] = BISHOP | BLACK;
    bb->pieces[18].piece_data = BISHOP | BLACK;
    bb->pieces[18].file = c;
    bb->pieces[18].rank = 8; 
    
    bb->squares[SQUARE(d, 8)] = QUEEN | BLACK;
    bb->pieces[19].piece_data = QUEEN | BLACK;
    bb->pieces[19].file = d;
    bb->pieces[19].rank = 8;
    
    bb->squares[SQUARE(e, 8)] = KING | BLACK;
    SET_CASTLE_SHORT(bb->squares[SQUARE(e, 8)]);
    SET_CASTLE_LONG(bb->squares[SQUARE(e, 8)]);

    bb->pieces[20].piece_data = bb->squares[SQUARE(e, 8)];
    bb->pieces[20].file = e;
    bb->pieces[20].rank = 8;

    bb->squares[SQUARE(f, 8)] = BISHOP | BLACK;
    bb->pieces[21].piece_data = BISHOP | BLACK;
    bb->pieces[21].file = f;
    bb->pieces[21].rank = 8;
    
    bb->squares[SQUARE(g, 8)] = KNIGHT | BLACK;
    bb->pieces[22].piece_data = KNIGHT | BLACK;
    bb->pieces[22].file = g;
    bb->pieces[22].rank = 8;

    bb->squares[SQUARE(h, 8)] = ROOK | BLACK;
    bb->pieces[23].piece_data = ROOK | BLACK;
    bb->pieces[23].file = h;
    bb->pieces[23].rank = 8;

    for (unsigned char i = 0; i < 8; ++i) {
        bb->squares[SQUARE(i, 7)] = PAWN | BLACK;
        bb->pieces[24 + i].piece_data = PAWN | BLACK;
        bb->pieces[24 + i].file = i;
        bb->pieces[24 + i].rank = 7;
    }
    
    bb->num_uncaptured = 32;
    bb->move = WHITE;

    return bb;
}

board *load_fen(char *fen) {
    
    board *bb = (board *) malloc(sizeof(board));

    if (!bb) {
        return NULL;
    }
    memset(&bb->squares, 0, 64);
    memset(&bb->pieces, 0, 32 * sizeof(piece));

    bb->num_uncaptured = 0;
    bb->move = WHITE;

    unsigned short i = 0;
    char rank = 8, file = 0;

    piece *white_king = NULL, *black_king = NULL;

    
    uint8_t segment_lengths[4] = {0, 0, 0, 0};
    char mode = 0;
    for (unsigned short j = 0, k = strlen(fen), l = 0; j < k; ++j) {
        if (fen[j] == ' ') {
            while (fen[j + 1] == ' ') {
                ++j;
            }
            ++l;
        } else {
            ++segment_lengths[l];
        }
    }
    
    char *fen_segments[4];
    
    for (uint8_t i = 0; i < 4; ++i) {
        fen_segments[i] = (char *) malloc(segment_lengths[i] + 1);

        if (!fen_segments[i]) {
            for (unsigned char j = 0; j < i; ++j) {
                free(fen_segments[j]);
                free(bb);
                return NULL;
            }
        }
        fen_segments[i][segment_lengths[i]] = '\0';

    }

    for (uint16_t j = 0, k = strlen(fen), l = 0, n = 0; j < k; ++j) {
        if (fen[j] == ' ') {
            while (fen[j + 1] == ' ') {
                ++j;
            }
            ++l;
            n = 0;
        } else {
            fen_segments[l][n] = fen[j];
            ++n;
        }
    }

    while (fen_segments[0][i]) {
        bool add_piece = true;
        if (fen_segments[0][i] >= '0' && fen_segments[0][i] <= '9') {
            file += fen_segments[0][i] - '0';
            ++i;
            continue;
        } else switch (fen_segments[0][i]) {
            case '/':
                --rank;
                file = -1;
                add_piece = false;
            break;
            case 'p':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | PAWN;
            break; 
            case 'r':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | ROOK;
            break;
            case 'n':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | KNIGHT;
            break;
            case 'b':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | BISHOP;
            break;
            case 'q':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | QUEEN;
            break;
            case 'k':
                bb->pieces[bb->num_uncaptured].piece_data = BLACK | KING;
                black_king = &bb->pieces[bb->num_uncaptured];
            break;
            case 'P':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | PAWN;
            break;
            case 'R':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | ROOK;
            break;
            case 'N':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | KNIGHT;
            break; 
            case 'B':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | BISHOP;
            break; 
            case 'Q':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | QUEEN;
            break; 
            case 'K':
                bb->pieces[bb->num_uncaptured].piece_data = WHITE | KING;
                white_king = &bb->pieces[bb->num_uncaptured];
            break;
            default:
                for (uint8_t j = 0; j < 4; ++j) {
                    free(fen_segments[j]);
                }
                free(bb);
                printf("Failed to load FEN: Invalid FEN character '%c'\n", fen_segments[0][i]);
                return NULL;
        }

        if (add_piece) {
            bb->pieces[bb->num_uncaptured].rank = rank;
            bb->pieces[bb->num_uncaptured].file = file;
            ++(bb->num_uncaptured);
        }

        ++i;
        ++file;
    }
    
    i = 0;

    if (!white_king || !black_king) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        
        free(bb);
        printf("Failed to load FEN: Invalid board position. White king or black king missing!\n");
        return NULL;
    }

    if (strlen(fen_segments[1]) != 1) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        free(bb);
        printf("Failed to load FEN: Move specifier should be 'w' for white or 'b' for black!\n");
        return NULL;
    } else if (fen_segments[1][0] == 'w') {
        bb->move = WHITE;
    } else if (fen_segments[1][0] == 'b') {
        bb->move = BLACK;
    } else {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        free(bb);
        printf("Failed to load FEN: Move specifier should be 'w' for white or 'b' for black!\n");
        return NULL;
    }
    
    i = 0;
    char *wk_pd = &(white_king->piece_data), *bk_pd = &(black_king->piece_data);
    while (fen_segments[2][i]) {
        switch (fen_segments[2][i]) {
            case 'K':
                SET_CASTLE_SHORT(*wk_pd);
            break;
            case 'Q':
                SET_CASTLE_LONG(*wk_pd);
            break; 

            case 'k':
                SET_CASTLE_SHORT(*bk_pd);
            break; 

            case 'q':
                SET_CASTLE_LONG(*bk_pd);
            break;
            case '-':
            break;
            default:
                for (uint8_t j = 0; j < 4; ++j) {
                    free(fen_segments[j]);
                }
                free(bb);
                printf("Failed to load FEN: Failed to load castling rights!\n");
                return NULL;
        }
        ++i;
    }

    for (i = 0; i < bb->num_uncaptured; ++i) {
        bb->squares[SQUARE(bb->pieces[i].file, bb->pieces[i].rank)] = bb->pieces[i].piece_data;
    }

    if (strlen(fen_segments[3]) != 2) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        free(bb);
        printf("Failed to load FEN: Must specify en-passant opportunities. \"--\" for none.");
        return NULL;
    }

    if (strcmp(fen_segments[3], "--") != 0) {
        rank = fen_segments[3][1] - '0';
        // if black to move, add one, if white to move - 1
        rank += 1 + ((bb->move == WHITE) * -2);
        file = fen_segments[3][0] - 'a';
        char piece_index = SQUARE(file, rank);
        if (piece_index < 0 || piece_index > 63) {
            
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            free(bb);
            printf("Failed to load FEN: Invalid coordinate for en-passant specifier.\n");
            return NULL;
        }

        if (PIECE_TYPE(bb->squares[piece_index]) != PAWN) {
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            free(bb);
            printf("Failed to load FEN: Invalid coordinate for en-passant specifier.\n");
            return NULL;
        }

        if (IS_BLACK(bb->squares[piece_index]) == (bb->move == BLACK)) {
            
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            free(bb);
            printf("Failed to load FEN: Invalid coordinate for en-passant specifier.\n");
            return NULL;
        }
        
        if (!((IS_BLACK(bb->squares[piece_index]) && rank == 5) || (IS_WHITE(bb->squares[piece_index]) && rank == 4))) {
            
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            free(bb);
            printf("Failed to load FEN: Invalid coordinate for en-passant specifier.\n");
            return NULL;
        }
        
        SET_PAWN_MOVED_TWO(bb->squares[piece_index]);

        for (i = 0; i < bb->num_uncaptured; ++i) {
            if (bb->pieces[i].rank == rank && bb->pieces[i].file == file) {
                bb->pieces[i].piece_data = bb->squares[piece_index];
            }
        }
    }


    for (uint8_t j = 0; j < 4; ++j) {
         free(fen_segments[j]);
    }
    printf("Successfully loaded FEN!\n");
    return bb;
}

void print_board(board *bb) {
    if (bb->move == BLACK) {
        printf("Black to move!\n");
    } else {
        printf("White to move!\n");
    }

    for (char rank = 8; rank >= 1; --rank) {
        print_rank(bb, rank);
    }
    for (char i = 0; i < 7; ++i) {
        printf("------");
    }
    printf("\n");
}

static void print_rank(board *bb, char rank) {
    for (char i = 0; i < 7; ++i) {
        printf("------");
    }
    
    printf("\n||");

    for (char file = 0; file < 8; ++file) {
        if ((rank + file) % 2 == 0) {
            // white square
            printf("   ");
        } else {
            // black square
            printf("***");
        }
        printf("||");
    }
    printf("\n||");
    for (char file = 0; file < 8; ++file) {
        if ((rank + file) % 2 == 0) {
            // white square
            printf(" ");
            printf("%c", get_piece_char(bb, file, rank));
            printf(" ");
        } else {
            // black square
            printf("*");
            printf("%c", get_piece_char(bb, file, rank));
            printf("*");
        }
        printf("||");
    }
    printf("\n||");
    for (char file = 0; file < 8; ++file) {
        if ((rank + file) % 2 == 0) {
            // white square
            printf("   ");
        } else {
            // black square
            printf("***");
        }
        printf("||");
    }
    printf("\n");
}