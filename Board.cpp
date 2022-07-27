#include <cstring>
#include "Board.h"
#include "King.h"
#include "Queen.h"
#include "Bishop.h"
#include "Rook.h"
#include "Knight.h"
#include "Pawn.h"

Board::Board(const char *fen) {
    uint32_t i = 0;
    char rank = 8, file = 0;

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
                return;
            }
        }
        fen_segments[i][segment_lengths[i]] = '\0';

    }

    for (uint32_t j = 0, k = strlen(fen), l = 0, n = 0; j < k; ++j) {
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

    white_king = nullptr;
    black_king = nullptr;

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
                black_pieces.push_back(new Pawn(BLACK, file, rank, this));
            break;
            case 'r':
                black_pieces.push_back(new Rook(BLACK, file, rank, this));
            break;
            case 'n':
                black_pieces.push_back(new Knight(BLACK, file, rank, this));
            break;
            case 'b':
                black_pieces.push_back(new Bishop(BLACK, file, rank, this));
            break;
            case 'q':
                black_pieces.push_back(new Queen(BLACK, file, rank, this));
            break;
            case 'k':
                black_king = new King(BLACK, file, rank, this);
                black_pieces.push_back(black_king);
            break;
            case 'P':
                white_pieces.push_back(new Pawn(WHITE, file, rank, this));
            break;
            case 'R':
                white_pieces.push_back(new Rook(WHITE, file, rank, this));
            break;
            case 'N':
                white_pieces.push_back(new Knight(WHITE, file, rank, this));
            break;
            case 'B':
                white_pieces.push_back(new Bishop(WHITE, file, rank, this));
            break;
            case 'Q':
                white_pieces.push_back(new Queen(WHITE, file, rank, this));
            break;
            case 'K':
                white_king = new King(WHITE, file, rank, this);
                white_pieces.push_back(white_king);
            break;
            default:
                for (uint8_t j = 0; j < 4; ++j) {
                    free(fen_segments[j]);
                }
                std::cout << "Failed to load FEN: Invalid FEN character " << fen_segments[0][i] << std::endl;
                return;
        }
        ++i;
        ++file;
    }

    i = 0;

    if (!white_king || !black_king) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        std::cout << "Failed to load FEN: Invalid board position. White king or black king missing!" << std::endl;
        return;
    }

    if (strlen(fen_segments[1]) != 1) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        std::cout << "Failed to load FEN: Move specifier should be 'w' for white or 'b' for black!" << std::endl;
        return;
    } else if (fen_segments[1][0] == 'w') {
        move = WHITE;
    } else if (fen_segments[1][0] == 'b') {
        move = BLACK;
    } else {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        std::cout << "Failed to load FEN: Move specifier should be 'w' for white or 'b' for black!" << std::endl;
        return;
    }
    i = 0;

    while (fen_segments[2][i]) {
        switch (fen_segments[2][i]) {
            case 'K':
                white_king->short_castle_rights = true;
            break;
            case 'Q':
                white_king->long_castle_rights = true;
            break;
            case 'k':
                black_king->short_castle_rights = true;
            break;

            case 'q':
                black_king->long_castle_rights = true;
            break;
            case '-':
            break;
            default:
                for (uint8_t j = 0; j < 4; ++j) {
                    free(fen_segments[j]);
                }
                std::cout << "Failed to load FEN: Failed to load castling rights!" << std::endl;
                return;
        }
        ++i;
    }

    if (strlen(fen_segments[3]) != 2) {
        for (uint8_t j = 0; j < 4; ++j) {
            free(fen_segments[j]);
        }
        std::cout << "Failed to load FEN: Must specify en-passant opportunities. \"--\" for none." << std::endl;
        return;
    }

    if (strcmp(fen_segments[3], "--") != 0) {
        rank = fen_segments[3][1] - '0';
        rank += 1 + ((move == WHITE) * -2);
        file = fen_segments[3][0] - 'a';
        char piece_index = offset(file, rank);
        if (piece_index < 0 || piece_index > 63) {
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            std::cout << "Failed to load FEN: Invalid coordinate for en-passant specifier." << std::endl;
            return;
        }

        if (!dynamic_cast<Pawn *>(squares[piece_index])) {
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            std::cout << "Failed to load FEN: Invalid coordinate for en-passant specifier." << std::endl;
            return;
        }

        if ((squares[piece_index]->color == BLACK) == (move == BLACK)) {
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            std::cout << "Failed to load FEN: Invalid coordinate for en-passant specifier." << std::endl;
            return;
        }

        if (!((squares[piece_index]->color == BLACK && rank == 5) || (squares[piece_index]->color == WHITE && rank == 4))) {
            for (uint8_t j = 0; j < 4; ++j) {
                free(fen_segments[j]);
            }
            std::cout << "Failed to load FEN: Invalid coordinate for en-passant specifier.\n" << std::endl;
            return;
        }

        Pawn *pawn = (Pawn *) squares[piece_index];
        pawn->moved_two = true;
    }

    memset((void *) squares, 0, sizeof(Piece *) * 64);
    prev_jmp_pawn = nullptr;
    for (Piece *p : white_pieces) {
        squares[offset(p->file, p->rank)] = p;
    }

    for (Piece *p : black_pieces) {
        squares[offset(p->file, p->rank)] = p;
    }

    for (uint8_t j = 0; j < 4; ++j) {
         free(fen_segments[j]);
    }
    std::cout << "Successfully loaded FEN!" << std::endl;
}

Board::Board() {
    for (char i = 0; i < 8; ++i) {
        white_pieces.push_back(new Pawn(WHITE, i, 2, this));
    }
    for (char i = 0; i < 8; ++i) {
        black_pieces.push_back(new Pawn(BLACK, i, 7, this));
    }
    white_pieces.push_back(new Rook(WHITE, A_FILE, 1, this));
    white_pieces.push_back(new Knight(WHITE, B_FILE, 1, this));
    white_pieces.push_back(new Bishop(WHITE, C_FILE, 1, this));
    white_pieces.push_back(new Queen(WHITE, D_FILE, 1, this));
    King *wk = new King(WHITE, E_FILE, 1, this);
    wk->short_castle_rights = true;
    wk->long_castle_rights = true;
    white_king = wk;
    white_pieces.push_back(wk);
    white_pieces.push_back(new Bishop(WHITE, F_FILE, 1, this));
    white_pieces.push_back(new Knight(WHITE, G_FILE, 1, this));
    white_pieces.push_back(new Rook(WHITE, H_FILE, 1, this));

    black_pieces.push_back(new Rook(BLACK, A_FILE, 8, this));
    black_pieces.push_back(new Knight(BLACK, B_FILE, 8, this));
    black_pieces.push_back(new Bishop(BLACK, C_FILE, 8, this));
    black_pieces.push_back(new Queen(BLACK, D_FILE, 8, this));
    King *bk = new King(BLACK, E_FILE, 8, this);
    bk->short_castle_rights = true;
    bk->long_castle_rights = true;
    black_king = bk;
    black_pieces.push_back(bk);
    black_pieces.push_back(new Bishop(BLACK, F_FILE, 8, this));
    black_pieces.push_back(new Knight(BLACK, G_FILE, 8, this));
    black_pieces.push_back(new Rook(BLACK, H_FILE, 8, this));

    prev_jmp_pawn = nullptr;
    move = Board::WHITE;
    memset((void *) squares, 0, sizeof(Piece *) * 64);
    for (int i = 0; i < white_pieces.size(); ++i) {
        Piece *p = white_pieces.at(i);
        squares[offset(p->file, p->rank)] = p;
    }

    for (int i = 0; i < black_pieces.size(); ++i) {
        Piece *p = black_pieces.at(i);
        squares[offset(p->file, p->rank)] = p;
    }
}

 Piece *Board::inspect(char file, char rank)  {
    return squares[(rank - 1) * 8 + file];
}

char Board::offset(char file, char rank) {
    return (rank - 1) * 8 + file;
}

 std::vector<Piece *> *Board::get_opposite_pieces(Color color)  {
    if (color == WHITE) {
        return &black_pieces;
    }
    return &white_pieces;
}

std::vector<Piece *> *Board::get_current_pieces() {
    if (move == WHITE) {
        return &white_pieces;
    }
    return &black_pieces;
}

std::vector<Piece *> *Board::get_white_pieces() {
    return &white_pieces;
}

std::vector<Piece *> *Board::get_black_pieces() {
    return &black_pieces;
}

King *Board::get_my_king(Color color) {
    if (color == WHITE) {
        return white_king;
    }
    return black_king;
}

std::vector<uint32_t> *Board::generate_moves() {
    std::vector<uint32_t> *move_list = new std::vector<uint32_t>();
    if (move == WHITE) {
        for (Piece *p : white_pieces) {
            if (p->is_taken) {
                continue;
            }
            p->add_moves(move_list);
        }
    } else {
        for (Piece *p : black_pieces) {
            if (p->is_taken) {
                continue;
            }
            p->add_moves(move_list);
        }
    }
    remove_illegal_moves(move_list);
    return move_list;
}

void Board::remove_illegal_moves(std::vector<uint32_t> *move_list) {
    Color move = this->move;
    King *king = get_my_king(this->move);
    for (size_t i = 0; i < move_list->size(); ++i) {
        uint32_t candidate_move = move_list->at(i);
        make_move(candidate_move);
        std::vector<Piece *> *opposite_pieces = get_current_pieces();
        for (Piece *opponent_piece : *opposite_pieces) {
            if (opponent_piece->can_attack(king->file, king->rank)) {
                /* King is in check as a result of the candidate move. Thus candidate move is illegal. */
                move_list->at(i) = move_list->back();
                move_list->pop_back();
                --i;
            }
        }
        revert_move();
    }
}

Pawn *Board::find_parent_pawn(Piece *promoted) {
    std::vector<Piece *> pieces;
    if (promoted->color == WHITE) {
        pieces = this->white_pieces;
    } else {
        pieces = this->black_pieces;
    }

    for (Piece *p : pieces) {
        Pawn *pawn;
        if ((pawn = dynamic_cast<Pawn *>(p)) && pawn->promoted_piece == promoted) {
            return pawn;
        }
    }
    return nullptr;
}

void Board::print_board() {
    if (move == BLACK) {
        std::cout << "Black to move!" << std::endl;
    } else {
        std::cout << "White to move!" << std::endl;
    }

    for (char rank = 8; rank >= 1; --rank) {
        print_rank(rank);
    }

    for (char i = 0; i < 7; ++i) {
        std::cout << "------";
    }
    std::cout << "\n";
}

void Board::print_rank(char rank) {
    for (char i = 0; i < 7; ++i) {
        std::cout << ("------");
    }

    std::cout << ("\n||");

    for (char file = 0; file < 8; ++file) {
        if ((rank + file) % 2 == 0) {
            // white square
            std::cout << ("   ");
        } else {
            // black square
            std::cout << ("***");
        }
        std::cout << ("||");
    }
    std::cout << ("\n||");
    for (char file = 0; file < 8; ++file) {

        Piece *p = squares[offset(file, rank)];
        if ((rank + file) % 2 == 0) {
            // white square
            std::cout << (" ");
            if (p) {
                std::cout << p->get_piece_char();
            } else {
                std::cout << ' ';
            }
            std::cout << (" ");
        } else {
            // black square
            std::cout << ("*");
            if (p) {
                std::cout << p->get_piece_char();
            } else {
                std::cout << '*';
            }
            std::cout << ("*");
        }
        std::cout << ("||");
    }
    std::cout << ("\n||");
    for (char file = 0; file < 8; ++file) {
        if ((rank + file) % 2 == 0) {
            // white square
            std::cout << ("   ");
        } else {
            // black square
            std::cout << ("***");
        }
        std::cout << ("||");
    }
    std::cout << ("\n");
}

void Board::print_move(uint32_t move) {
    /* */
    char piece_moved = GET_PIECE_MOVED(move);
    switch (piece_moved) {
        case QUEEN:
            std::cout << "Q";
        break;
        case ROOK:
            std::cout << "R";
        break;
        case BISHOP:
            std::cout << "B";
        break;
        case KNIGHT:
            std::cout << "N";
        break;
        case PAWN:
        break;
        case KING:
            if (GET_SHORT_CASTLING(move)) {
                std::cout << "O-O";
                return;
            } else if (GET_LONG_CASTLING(move)) {
                std::cout << "O-O-O";
                return;
            } else {
                std::cout << "K";
            }
        break;
    }

    if (GET_IS_CAPTURE(move) || GET_IS_ENPASSANT(move)) {
        if (piece_moved == PAWN) {
            std::cout << (char) (GET_FROM_FILE(move) + 'a');
        }
        std::cout << "x";
    }
    std::cout << (char) (GET_TO_FILE(move) + 'a') << GET_TO_RANK(move);

    if (GET_IS_PROMOTION(move)) {
        std::cout << "=";
        switch (GET_PROMOTION_PIECE(move)) {
            case QUEEN:
                std::cout << "Q";
            break;
            case ROOK:
                std::cout << "K";
            break;
            case KNIGHT:
                std::cout << "N";
            break;
            case BISHOP:
                std::cout << "B";
            break;
        }
    }
}

void Board::make_move(uint32_t move) {
    if (prev_jmp_pawn) {
        prev_jmp_pawn->moved_two = false;
    }

    char ff = GET_FROM_FILE(move), fr = GET_FROM_RANK(move);
    char tf = GET_TO_FILE(move), tr = GET_TO_RANK(move);

    Piece *mp = squares[offset(ff, fr)];
    squares[offset(ff, fr)] = nullptr;

    if (GET_IS_CAPTURE(move)) {
        Piece *cp = squares[offset(tf, tr)];
        squares[offset(tf, tr)] = nullptr;
        captured_pieces.push(cp);
        cp->is_taken = true;
    }

    if (GET_REM_LCASTLE(move)) {
        King *my_king = get_my_king(this->move);
        my_king->long_castle_rights = false;
    }
    if (GET_REM_SCASTLE(move)) {
        King *my_king = get_my_king(this->move);
        my_king->short_castle_rights = false;
    }

    if (GET_SHORT_CASTLING(move)) {
        Rook *rook = dynamic_cast<Rook *>(inspect(H_FILE, fr));
        rook->file = F_FILE;
        squares[offset(H_FILE, fr)] = nullptr;
        squares[offset(F_FILE, fr)] = rook;
    } else if (GET_LONG_CASTLING(move)) {
        Rook *rook = dynamic_cast<Rook *> (inspect(A_FILE, fr));
        rook->file = D_FILE;
        squares[offset(A_FILE, fr)] = nullptr;
        squares[offset(D_FILE, fr)] = rook;
    }

    if (GET_PIECE_MOVED(move) == PAWN) {
        Pawn *pawn = dynamic_cast<Pawn *>(mp);
        if (GET_IS_PROMOTION(move)) {
            switch (GET_PROMOTION_PIECE(move)) {
                case QUEEN:
                    pawn->promoted_piece = new Queen(pawn->color, tf, tr, this);
                break;
                case ROOK:
                    pawn->promoted_piece = new Rook(pawn->color, tf, tr, this);
                break;
                case BISHOP:
                    pawn->promoted_piece = new Bishop(pawn->color, tf, tr, this);
                break;
                case KNIGHT:
                    pawn->promoted_piece = new Knight(pawn->color, tf, tr, this);
                break;
            }
            mp = pawn->promoted_piece;
        } else if (abs(tr - fr) == 2) {
            pawn->moved_two = true;
            prev_jmp_pawn = pawn;
        } else if (GET_IS_ENPASSANT(move)) {
            Piece *cp = squares[offset(tf, tr - pawn->get_direction())];
            squares[offset(tf, tr - pawn->get_direction())] = nullptr;
            captured_pieces.push(cp);
            cp->is_taken = true;
        }
    }

    squares[offset(tf, tr)] = mp;
    mp->file = tf;
    mp->rank = tr;

    this->move_stack.push(move);
    if (this->move == WHITE) {
        this->move = BLACK;
    } else {
        this->move = WHITE;
    }
}

uint32_t Board::revert_move() {
    if (this->move == WHITE) {
        this->move = BLACK;
    } else {
        this->move = WHITE;
    }

    const uint32_t prev_move = move_stack.top();
    move_stack.pop();

    char ff = GET_FROM_FILE(prev_move), fr = GET_FROM_RANK(prev_move);
    char tf = GET_TO_FILE(prev_move), tr = GET_TO_RANK(prev_move);

    Piece *mp = squares[offset(tf, tr)];
    squares[offset(tf, tr)] = nullptr;

    if (GET_IS_CAPTURE(prev_move)) {
        Piece *cp = captured_pieces.top();
        captured_pieces.pop();
        cp->is_taken = false;
        squares[offset(tf, tr)] = cp;
    }

    if (GET_REM_LCASTLE(prev_move)) {
        King *my_king = get_my_king(this->move);
        my_king->long_castle_rights = true;
    }

    if (GET_REM_SCASTLE(prev_move)) {
        King *my_king = get_my_king(this->move);
        my_king->short_castle_rights = true;
    }

    if (GET_SHORT_CASTLING(prev_move)) {
        Rook *rook = dynamic_cast<Rook *> (inspect(F_FILE, tr));
        squares[offset(F_FILE, tr)] = nullptr;
        rook->file = H_FILE;
        squares[offset(H_FILE, tr)] = rook;
    } else if (GET_LONG_CASTLING(prev_move)) {
        Rook *rook = dynamic_cast<Rook *> (inspect(D_FILE, tr));
        squares[offset(D_FILE, tr)] = nullptr;
        rook->file = A_FILE;
        squares[offset(A_FILE, tr)] = rook;
    }

    if (GET_PIECE_MOVED(prev_move) == PAWN) {
        if (GET_IS_PROMOTION(prev_move)) {
            Pawn *parent_pawn = find_parent_pawn(mp);
            delete parent_pawn->promoted_piece;
            parent_pawn->promoted_piece = nullptr;
            mp = parent_pawn;
        } else if (abs(tr - fr) == 2) {
            Pawn *pawn = dynamic_cast<Pawn *>(mp);
            pawn->moved_two = false;
        } else if (GET_IS_ENPASSANT(prev_move)) {
            Pawn *pawn = dynamic_cast<Pawn *>(mp);
            Piece *cp = captured_pieces.top();
            captured_pieces.pop();
            cp->is_taken = false;
            squares[offset(tf, tr - pawn->get_direction())] = cp;
        }
    }

    if (!move_stack.empty()) {
        const uint32_t pprev_move = move_stack.top();
        if (GET_PIECE_MOVED(pprev_move) == PAWN && abs((char) (GET_FROM_RANK(pprev_move) - GET_TO_RANK(pprev_move))) == 2) {
            Pawn *pawn = dynamic_cast<Pawn *>(squares[offset(GET_TO_FILE(pprev_move), GET_TO_RANK(pprev_move))]);
            pawn->moved_two = true;
            prev_jmp_pawn = pawn;
        }
    }

    squares[offset(ff, fr)] = mp;
    mp->file = ff;
    mp->rank = fr;

    return prev_move;
}