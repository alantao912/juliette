//
// Created by Alan Tao on 10/26/2022.
//

#pragma once

#include <cstdint>

#define QUEEN_MATERIAL 900
#define ROOK_MATERIAL 500
#define BISHOP_MATERIAL 325
#define KNIGHT_MATERIAL 300
#define PAWN_MATERIAL 100

#define QUEEN_MATERIAL_E 900
#define ROOK_MATERIAL_E 500
#define BISHOP_MATERIAL_E 325
#define KNIGHT_MATERIAL_E 300
#define PAWN_MATERIAL_E 100

#define CONNECTED_PAWN_BONUS 5
#define P5 50
#define P6 100
#define P7 150

#define DOUBLED_PAWN_PENALTY 20

#define CONNECTED_PAWN_BONUS_E 5
#define RANK_5_PAWN_BONUS_E 50
#define RANK_6_PAWN_BONUS_E 150
#define RANK_7_PAWN_BONUS_E 250

#define DOUBLED_PAWN_PENALTY_E 20

#define HIT_BONUS 5
#define HIT_BONUS_E 3

#define CONNECTED_ROOK_BONUS 40
#define CONNECTED_ROOK_BONUS_E 80

#define ROOK_ON_7_BONUS 100
#define ROOK_ON_7_BONUS_E 200

#define QQ_BATTERY 100
#define QQ_BATTERY_E 60

#define QR_BATTERY  80
#define QR_BATTERY_E 40

#define QB_BATTERY   90
#define QB_BATTERY_E 60

namespace Weights {
    int32_t king_psqt[] = {

    };

    int32_t queen_psqt[] = {

    };

    int32_t rook_psqt[] = {

    };

    int32_t bishop_psqt[] = {

    };

    int32_t knight_psqt[] = {
            -100, -80, -80, -80, -80, -80, -80, -100,
            -80, -80, 0, 10, 10, 0, -80, -80,
            -30, -10, 800000, 0, 0, 800000, -10, -30,
            -10, 0, 10, 20, 20, 10, 0, -10,
            0, 10, 20, 30, 30, 20, 10, 0,
            0, 10, 20, 30, 30, 20, 10, 0,
            -10, 0, 10, 20, 20, 10, 0, -10,
            0, 0, 0, 0, 0, 0, 0, 0
    };

    int32_t pawn_psqt[] = {
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 10, 0, 20, 20, 0, 10, 0,
            P5, P5, P5, P5, P5, P5, P5, P5,
            P6, P6, P6, P6, P6, P6, P6, P6,
            P7, P7, P7, P7, P7, P7, P7, P7,
            0, 0, 0, 0, 0, 0, 0, 0
    };
}