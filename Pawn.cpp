#include "Pawn.h"

Pawn::Pawn(Board::Color color, char file, char rank, Board *parent) : Piece(color, file, rank, parent) {
    if (color == Board::WHITE) {
        direction_offset = 1;
        starting_rank = 2;
        promotion_rank = 8;
        en_passant_rank = 5;
    } else {
        direction_offset = -1;
        starting_rank = 7;
        promotion_rank = 1;
        en_passant_rank = 4;
    }
    moved_two = false;
    promoted_piece = nullptr;
}

void Pawn::check_promotion(std::vector<uint32_t> *move_list, uint32_t move) {
    if (GET_TO_RANK(move) == promotion_rank) {
        move_list->push_back(move | IS_PROMOTION | PROMOTION_PIECE(KNIGHT));
        move_list->push_back(move | IS_PROMOTION | PROMOTION_PIECE(BISHOP));
        move_list->push_back(move | IS_PROMOTION | PROMOTION_PIECE(ROOK));
        move_list->push_back(move | IS_PROMOTION | PROMOTION_PIECE(QUEEN));
    } else {
        move_list->push_back(move);
    }
}

void Pawn::add_moves(std::vector<uint32_t> *move_list) {
    if (promoted_piece) {
        promoted_piece->add_moves(move_list);
        return;
    }
    uint32_t move;
    if (!(parent->inspect(file, rank + direction_offset))) {
        move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE(file) | TO_RANK((rank + direction_offset)) | SET_PIECE_MOVED(PAWN);
        check_promotion(move_list, move);
        if (rank == starting_rank && !(parent->inspect(file, rank + 2 * direction_offset))) {
            /* Pawn is on its starting rank, and the square two ranks in front of it is empty */
            move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE(file) | TO_RANK((rank + 2 * direction_offset)) | SET_PIECE_MOVED(PAWN);
            move_list->push_back(move);
        }
    }

    Piece *attack_square = nullptr;
    if (file != A_FILE) {
        /* Pawn is not on the A file, check if it can capture toward file - 1 */
        attack_square = parent->inspect(file - 1, rank + direction_offset);
        if (attack_square && attack_square->color != this->color) {
            /* If there is a piece on the attack square, and the piece's color is opposite to it's own */
            move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file - 1)) | TO_RANK((rank + direction_offset)) | IS_CAPTURE | SET_PIECE_MOVED(PAWN);
            check_promotion(move_list, move);
        } else if (rank == en_passant_rank && (attack_square = parent->inspect(file - 1, rank)) && attack_square->color != this->color) {
            /* If rank is on 5th rank (if white) or 4th rank (if black) and the square on file + 1 has a piece whose color is not equal to its own */
            Pawn *pawn = dynamic_cast<Pawn *>(attack_square);
            if (pawn && pawn->moved_two) {
                move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file - 1)) | TO_RANK((rank + direction_offset)) | IS_ENPASSANT | SET_PIECE_MOVED(PAWN);
                move_list->push_back(move);
            }
        }
    }

    if (file != H_FILE) {
        /* Pawn is not on the H file, check if it can capture towards file + 1 */
        attack_square = parent->inspect(file + 1, rank + direction_offset);
        if (attack_square && attack_square->color != this->color) {
            /* If there is a piece on the attack square, and the piece's color is opposite to it's own */
            move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file + 1)) | TO_RANK((rank + direction_offset)) | IS_CAPTURE | SET_PIECE_MOVED(PAWN);
            check_promotion(move_list, move);
        } else if (rank == en_passant_rank && (attack_square = parent->inspect(file + 1, rank)) && attack_square->color != this->color) {
            /* If rank is on 5th rank (if white) or 4th rank (if black) and the square on file + 1 has a piece whose color is not equal to its own */
            Pawn *pawn = dynamic_cast<Pawn *>(attack_square);
            if (pawn && pawn->moved_two) {
                move = 0 | FROM_FILE(file) | FROM_RANK(rank) | TO_FILE((file + 1)) | TO_RANK((rank + direction_offset)) | IS_ENPASSANT | SET_PIECE_MOVED(PAWN);
                move_list->push_back(move);
            }
        }
    }
}

bool Pawn::can_attack(char file, char rank) {
    if (promoted_piece) {
        return promoted_piece->can_attack(file, rank);
    }
    return (rank == this->rank + this->direction_offset) && (file == this->file + 1 || file == this->file - 1);
}

char Pawn::get_piece_char() {
    if (color == Board::BLACK) {
        return 'p';
    }
    return 'P';
}

char Pawn::get_type() {
    return PAWN;
}

char Pawn::get_direction() {
    return direction_offset;
}