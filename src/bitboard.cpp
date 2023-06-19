#include <cstring>

#include "bitboard.h"
#include "util.h"
#include "movegen.h"

extern __thread bitboard board;

void initialize_zobrist() {
    for (int i = 0; i < 781; ++i) {
        ZOBRIST_VALUES[i] = rand_bitstring();
    }
}

void init_board(const char *fen) {
    char *rest = strdup(fen);
    char *og_rest = rest;
    // Initalize bitboards and mailbox
    char *token = strtok_r(rest, " ", &rest);
    for (int i = A1; i <= H8; ++i) {
        board.mailbox[i] = EMPTY;
    }
    board.w_pawns = 0;
    board.w_knights = 0;
    board.w_bishops = 0;
    board.w_rooks = 0;
    board.w_queens = 0;
    board.w_king = 0;
    board.b_pawns = 0;
    board.b_knights = 0;
    board.b_bishops = 0;
    board.b_rooks = 0;
    board.b_queens = 0;
    board.b_king = 0;
    for (int rank = 7; rank >= 0; --rank) {
        char *fen_board = strtok_r(token, "/", &token);
        int file = 0;
        const size_t len = strlen(fen_board);
        for (int j = 0; j < len; ++j) {
            if (file >= 8) break;

            char piece = fen_board[j];
            if (isdigit(piece)) {
                file += piece - '0';
            } else {
                int square = 8 * rank + file;
                board.mailbox[square] = to_enum(piece);
                uint64_t *bitboard = get_bitboard(to_enum(piece));
                set_bit(bitboard, square);
                ++file;
            }
        }
    }
    board.w_occupied =
            board.w_pawns | board.w_knights | board.w_bishops | board.w_rooks | board.w_queens | board.w_king;
    board.b_occupied =
            board.b_pawns | board.b_knights | board.b_bishops | board.b_rooks | board.b_queens | board.b_king;
    board.occupied = board.w_occupied | board.b_occupied;

    // Initalize king squares
    board.w_king_square = get_lsb(board.w_king);
    board.b_king_square = get_lsb(board.b_king);

    // Initalize turn
    token = strtok_r(rest, " ", &rest);
    board.turn = (*token == 'w') ? WHITE : BLACK;

    // Initalize castling rights
    token = strtok_r(rest, " ", &rest);
    board.w_kingside_castling_rights = false;
    board.w_queenside_castling_rights = false;
    board.b_kingside_castling_rights = false;
    board.b_queenside_castling_rights = false;
    for (int i = 0, j = strlen(token); i < j; i++) {
        char piece = token[i];
        switch (piece) {
            case 'K':
                board.w_kingside_castling_rights = true;
                break;
            case 'Q':
                board.w_queenside_castling_rights = true;
                break;
            case 'k':
                board.b_kingside_castling_rights = true;
                break;
            case 'q':
                board.b_queenside_castling_rights = true;
                break;
        }
    }
    // Initalize possible en passant square
    token = strtok_r(rest, " ", &rest);
    board.en_passant_square = (*token == '-') ? INVALID : parse_square(token);
    // Initalize halfmove clock
    token = strtok_r(rest, " ", &rest);

    if (token) board.halfmove_clock = strtol(token, nullptr, 10);
    // Initalize fullmove number
    token = strtok_r(rest, " ", &rest);
    if (token) board.fullmove_number = strtol(token, nullptr, 10);
    board.hash_code = 0;
    for (int square = A1; square <= H8; square++) {
        piece_t piece = board.mailbox[square];
        if (piece != EMPTY) {
            board.hash_code ^= ZOBRIST_VALUES[64 * (int) piece + square];
        }
    }
    if (board.turn == BLACK) {
        board.hash_code ^= ZOBRIST_VALUES[768];
    }
    if (board.w_kingside_castling_rights) {
        board.hash_code ^= ZOBRIST_VALUES[769];
    }
    if (board.w_queenside_castling_rights) {
        board.hash_code ^= ZOBRIST_VALUES[770];
    }
    if (board.b_kingside_castling_rights) {
        board.hash_code ^= ZOBRIST_VALUES[771];
    }
    if (board.b_queenside_castling_rights) {
        board.hash_code ^= ZOBRIST_VALUES[772];
    }
    if (board.en_passant_square != INVALID) {
        board.hash_code ^= ZOBRIST_VALUES[773 + file_of(board.en_passant_square)];
    }
    free(og_rest);
}

/**
 * Updates the board with the move.
 * @param move
 */
void make_move(const move_t move) {
    int from = move.from;
    int to = move.to;
    int flag = move.flag;
    bool color = board.turn;

    piece_t attacker = board.mailbox[from];
    piece_t victim = board.mailbox[to];
    if (flag == PASS) {
        board.turn = !color;
        board.hash_code ^= ZOBRIST_VALUES[768];
        return;
    }

    bool reset_halfmove = false;
    if (board.en_passant_square != INVALID) {
        board.hash_code ^= ZOBRIST_VALUES[773 + file_of(board.en_passant_square)];
        board.en_passant_square = INVALID;
    }
    uint64_t *attacker_bb = get_bitboard(attacker);
    clear_bit(attacker_bb, from);
    set_bit(attacker_bb, to);
    board.mailbox[from] = EMPTY;
    board.mailbox[to] = attacker;
    board.hash_code ^= ZOBRIST_VALUES[64 * (int) attacker + from];
    board.hash_code ^= ZOBRIST_VALUES[64 * (int) attacker + to];

    switch (attacker) {
        case WHITE_PAWN:
            reset_halfmove = true;
            if (rank_of(to) - rank_of(from) == 2) {
                board.en_passant_square = to - 8;
                board.hash_code ^= ZOBRIST_VALUES[773 + file_of(board.en_passant_square)];
            } else if (flag == EN_PASSANT) {
                clear_bit(&board.b_pawns, to - 8);
                board.mailbox[to - 8] = EMPTY;
                board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_PAWN + (to - 8)];
            } else if (rank_of(to) == 7) { // Promotions
                clear_bit(&board.w_pawns, to);
                board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_PAWN + to];
                switch (flag) {
                    case PR_QUEEN:
                        set_bit(&board.w_queens, to);
                        board.mailbox[to] = WHITE_QUEEN;
                        board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_QUEEN + to];
                        break;
                    case PR_ROOK:
                        set_bit(&board.w_rooks, to);
                        board.mailbox[to] = WHITE_ROOK;
                        board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_ROOK + to];
                        break;
                    case PR_BISHOP:
                        set_bit(&board.w_bishops, to);
                        board.mailbox[to] = WHITE_BISHOP;
                        board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_BISHOP + to];
                        break;
                    case PR_KNIGHT:
                        set_bit(&board.w_knights, to);
                        board.mailbox[to] = WHITE_KNIGHT;
                        board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_KNIGHT + to];
                        break;
                }
            }
            break;
        case WHITE_ROOK:
            if (from == H1 && board.w_kingside_castling_rights) {
                board.w_kingside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[769];
            } else if (from == A1 && board.w_queenside_castling_rights) {
                board.w_queenside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[770];
            }
            break;
        case WHITE_KING:
            board.w_king_square = to;
            if (flag == CASTLING) {
                if (file_of(to) - file_of(from) > 0) { // Kingside
                    clear_bit(&board.w_rooks, H1);
                    set_bit(&board.w_rooks, F1);
                    board.mailbox[H1] = EMPTY;
                    board.mailbox[F1] = WHITE_ROOK;
                    board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_ROOK + H1];
                    board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_ROOK + F1];
                } else { // Queenside
                    clear_bit(&board.w_rooks, A1);
                    set_bit(&board.w_rooks, D1);
                    board.mailbox[A1] = EMPTY;
                    board.mailbox[D1] = WHITE_ROOK;
                    board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_ROOK + A1];
                    board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_ROOK + D1];
                }
            }

            if (board.w_kingside_castling_rights) {
                board.w_kingside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[769];
            }
            if (board.w_queenside_castling_rights) {
                board.w_queenside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[770];
            }
            break;
        case BLACK_PAWN:
            reset_halfmove = true;
            if (rank_of(to) - rank_of(from) == -2) {
                board.en_passant_square = to + 8;
                board.hash_code ^= ZOBRIST_VALUES[773 + file_of(board.en_passant_square)];
            } else if (flag == EN_PASSANT) {
                clear_bit(&board.w_pawns, to + 8);
                board.mailbox[to + 8] = EMPTY;
                board.hash_code ^= ZOBRIST_VALUES[64 * WHITE_PAWN + (to + 8)];
            } else if (rank_of(to) == 0) { // Promotions
                clear_bit(&board.b_pawns, to);
                board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_PAWN + to];
                switch (flag) {
                    case PR_QUEEN:
                        set_bit(&board.b_queens, to);
                        board.mailbox[to] = BLACK_QUEEN;
                        board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_QUEEN + to];
                        break;
                    case PR_ROOK:
                        set_bit(&board.b_rooks, to);
                        board.mailbox[to] = BLACK_ROOK;
                        board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_ROOK + to];
                        break;
                    case PR_BISHOP:
                        set_bit(&board.b_bishops, to);
                        board.mailbox[to] = BLACK_BISHOP;
                        board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_BISHOP + to];
                        break;
                    case PR_KNIGHT:
                        set_bit(&board.b_knights, to);
                        board.mailbox[to] = BLACK_KNIGHT;
                        board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_KNIGHT + to];
                        break;
                }
            }
            break;
        case BLACK_ROOK:
            if (from == H8 && board.b_kingside_castling_rights) {
                board.b_kingside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[771];
            } else if (from == A8 && board.b_queenside_castling_rights) {
                board.b_queenside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[772];
            }
            break;
        case BLACK_KING:
            board.b_king_square = to;
            if (flag == CASTLING) {
                if (file_of(to) - file_of(from) > 0) { // Kingside
                    clear_bit(&board.b_rooks, H8);
                    set_bit(&board.b_rooks, F8);
                    board.mailbox[H8] = EMPTY;
                    board.mailbox[F8] = BLACK_ROOK;
                    board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_ROOK + H8];
                    board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_ROOK + F8];
                } else { // Queenside
                    clear_bit(&board.b_rooks, A8);
                    set_bit(&board.b_rooks, D8);
                    board.mailbox[A8] = EMPTY;
                    board.mailbox[D8] = BLACK_ROOK;
                    board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_ROOK + A8];
                    board.hash_code ^= ZOBRIST_VALUES[64 * BLACK_ROOK + D8];
                }
            }

            if (board.b_kingside_castling_rights) {
                board.b_kingside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[771];
            }
            if (board.b_queenside_castling_rights) {
                board.b_queenside_castling_rights = false;
                board.hash_code ^= ZOBRIST_VALUES[772];
            }
            break;
    }
    if (victim != EMPTY) {
        reset_halfmove = true;
        uint64_t *victim_bb = get_bitboard(victim);
        clear_bit(victim_bb, to);
        board.hash_code ^= ZOBRIST_VALUES[64 * (int) victim + to];
    }
    board.w_occupied =
            board.w_pawns | board.w_knights | board.w_bishops | board.w_rooks | board.w_queens | board.w_king;
    board.b_occupied =
            board.b_pawns | board.b_knights | board.b_bishops | board.b_rooks | board.b_queens | board.b_king;
    board.occupied = board.w_occupied | board.b_occupied;
    if (reset_halfmove) {
        board.halfmove_clock = 0;
    } else {
        board.halfmove_clock++;
    }
    board.turn = !color;
    board.fullmove_number += color;
    board.hash_code ^= ZOBRIST_VALUES[768];
}

bool is_check(bool color) {
    if (color == WHITE) {
        return is_attacked(BLACK, get_lsb(board.w_king));
    } else {
        return is_attacked(WHITE, get_lsb(board.b_king));
    }
}

bool is_move_check(move_t move) {
    bitboard curr_board = board;
    make_move(move);
    bool i = is_check(board.turn);
    board = curr_board;
    return i;
}

/**
 * @param color the color of the attackers.
 * @param square the square potentially being attacked.
 * @return true if the square is being attacked by the given side
 */
bool is_attacked(bool color, int square) {
    if (color == BLACK) {
        uint64_t square_bb = BB_SQUARES[square];
        if (get_queen_moves(WHITE, square) & board.b_queens) return true;
        if (get_rook_moves(WHITE, square) & board.b_rooks) return true;
        if (get_bishop_moves(WHITE, square) & board.b_bishops) return true;
        if (get_knight_moves(WHITE, square) & board.b_knights) return true;
        if ((((square_bb << 9) & ~BB_FILE_A) | ((square_bb << 7) & ~BB_FILE_H)) & board.b_pawns) return true;
        return false;
    } else {
        uint64_t square_bb = BB_SQUARES[square];
        if (get_queen_moves(BLACK, square) & board.w_queens) return true;
        if (get_rook_moves(BLACK, square) & board.w_rooks) return true;
        if (get_bishop_moves(BLACK, square) & board.w_bishops) return true;
        if (get_knight_moves(BLACK, square) & board.w_knights) return true;
        if ((((square_bb >> 9) & ~BB_FILE_H) | ((square_bb >> 7) & ~BB_FILE_A)) & board.w_pawns) return true;
        return false;
    }
}

/**
 *
 */
uint64_t attacks_to(int target_square, uint64_t occupied_bb) {
    uint64_t attacks = 0ULL;
    uint64_t pieces = board.occupied;

    const uint64_t target_bb = BB_SQUARES[target_square];
    while (pieces) {
        int i = pull_lsb(&pieces);
        piece_t piece_type = static_cast<piece_t> ((static_cast<int> (board.mailbox[i]) % 6));
        switch (piece_type) {
            case BLACK_PAWN: {
                int pawn_file = file_of(i), pawn_rank = rank_of(i);
                int target_file = file_of(target_square), target_rank = rank_of(target_square);
                bool are_files_adjacent = abs(pawn_file - target_file) == 1;
                bool is_direction_correct = (target_rank - pawn_rank) == (1 - (2 * (board.mailbox[i] < 6)));
                uint64_t result = are_files_adjacent & is_direction_correct;
                attacks |= (result << i);
                break;
            }
            case BLACK_KNIGHT: {
                bool is_attacking = (BB_KNIGHT_ATTACKS[i] & target_bb) != 0;
                attacks |= (uint64_t(is_attacking) << i);
                break;
            }
            case BLACK_BISHOP: {
                uint64_t bishop_attacks = (BB_DIAGONALS[diagonal_of(i)] ^
                                           BB_ANTI_DIAGONALS[anti_diagonal_of(i)]);
                uint64_t ray = get_ray_between(i, target_square) & ~BB_SQUARES[i] & ~BB_SQUARES[target_square];
                bool alignment_exists = (bishop_attacks | target_bb) == bishop_attacks;
                bool no_obstructions = (ray ^ occupied_bb) == (ray | occupied_bb);
                uint64_t result = alignment_exists & no_obstructions;
                attacks |= (result << i);
                break;
            }
            case BLACK_ROOK: {
                uint64_t rook_attacks = (BB_RANKS[rank_of(i)] ^ BB_FILES[file_of(i)]);
                uint64_t ray = get_ray_between(i, target_square) & ~BB_SQUARES[i] & ~BB_SQUARES[target_square];
                bool alignment_exists = ((rook_attacks | target_bb) == rook_attacks);
                bool no_obstructions = (ray ^ occupied_bb) == (ray | occupied_bb);
                uint64_t result = alignment_exists & no_obstructions;
                attacks |= (result << i);
                break;
            }
            case BLACK_QUEEN: {
                uint64_t ray = get_ray_between(target_square, i);
                bool alignment_exists = ray != 0;
                ray &= ~BB_SQUARES[i];
                ray &= ~BB_SQUARES[target_square];
                bool no_obstructions = (ray ^ occupied_bb) == (ray | occupied_bb);
                uint64_t result = alignment_exists & no_obstructions;
                attacks |= (result << i);
                break;
            }
            case BLACK_KING: {
                bool can_king_reach = (BB_KING_ATTACKS[i] & target_bb) != 0;
                uint64_t result = can_king_reach;
                attacks |= (result << i);
                break;
            }
            default:
                std::cout << "Something is seriously wrong in attacks_to() in bitboard.cpp\n";
        }
    }
    return attacks;
}


/**
 * @param piece
 * @return a pointer to the bitboard of the piece.
 */
uint64_t *get_bitboard(piece_t piece) {
    switch (piece) {
        case WHITE_PAWN:
            return &board.w_pawns;
        case WHITE_KNIGHT:
            return &board.w_knights;
        case WHITE_BISHOP:
            return &board.w_bishops;
        case WHITE_ROOK:
            return &board.w_rooks;
        case WHITE_QUEEN:
            return &board.w_queens;
        case WHITE_KING:
            return &board.w_king;
        case BLACK_PAWN:
            return &board.b_pawns;
        case BLACK_KNIGHT:
            return &board.b_knights;
        case BLACK_BISHOP:
            return &board.b_bishops;
        case BLACK_ROOK:
            return &board.b_rooks;
        case BLACK_QUEEN:
            return &board.b_queens;
        case BLACK_KING:
            return &board.b_king;
        default:
            return nullptr;
    }
}

void print_bitboard(uint64_t bb) {
    uint64_t iterator = 1;
    char str[64];
    memset(str, 0, 64);
    int i = 0;
    while (iterator != 0) {
        if (bb & iterator) {
            str[i] = '1';
        } else {
            str[i] = '0';
        }
        ++i;
        iterator <<= 1;
    }

    for (int r = 7; r >= 0; --r) {
        for (int c = 0; c < 8; ++c) {
            int j = r * 8 + c;
            std::cout << (char) str[j];
        }
        std::cout << '\n';
    }
}

/**
 * Prints the labeled representation of the mailbox board.
 */
void print_board() {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            std::cout << to_char(board.mailbox[8 * rank + file]) << ' ';
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

move_t parse_move(const std::string &mv_str) {
    if (mv_str.size() < 4) {
        return NULL_MOVE;
    }
    move_t moves[MAX_MOVE_NUM];
    int n = gen_legal_moves(moves, board.turn);
    for (int i = 0; i < n; ++i) {
        int src_file = int(mv_str[0] - 'a');
        int src_rank = int(mv_str[1] - '1');
        int dest_file = int(mv_str[2] - 'a');
        int dest_rank = int(mv_str[3] - '1');

        if ((moves[i].flag > CAPTURE) && (mv_str.size() < 5)) {
            /** Some sort of promotion is happening. However, the move string does not contain a 5th character */
            continue;
        }

        if (moves[i].from == 8 * src_rank + src_file && moves[i].to == 8 * dest_rank + dest_file) {
            if (moves[i].flag == NONE) {
                return moves[i];
            } else if ((moves[i].flag == PC_QUEEN || moves[i].flag == PR_QUEEN) && mv_str[4] == 'q') {
                return moves[i];
            } else if ((moves[i].flag == PC_ROOK || moves[i].flag == PR_ROOK) && mv_str[4] == 'r') {
                return moves[i];
            } else if ((moves[i].flag == PC_BISHOP || moves[i].flag == PR_BISHOP) && mv_str[4] == 'b') {
                return moves[i];
            } else if ((moves[i].flag == PC_KNIGHT || moves[i].flag == PR_KNIGHT) && mv_str[4] == 'n') {
                return moves[i];
            }
        }
    }
    return NULL_MOVE;
}