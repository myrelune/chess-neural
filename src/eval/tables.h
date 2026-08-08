#pragma once

#include "../board/bitboard.h"

namespace Tables {

extern const int mgValue[6];
extern const int egValue[6];

extern const int mg_pawn_table[64];
extern const int eg_pawn_table[64];

extern const int mg_knight_table[64];
extern const int eg_knight_table[64];

extern const int mg_bishop_table[64];
extern const int eg_bishop_table[64];

extern const int mg_rook_table[64];
extern const int eg_rook_table[64];

extern const int mg_queen_table[64];
extern const int eg_queen_table[64];

extern const int mg_king_table[64];
extern const int eg_king_table[64];

extern const int* mgPestoTable[6];
extern const int* egPestoTable[6];

extern int mgTable[13][64];
extern int egTable[13][64];

extern const int gamePhaseInc[13];

extern Bitboard whitePassedMask[64];
extern Bitboard blackPassedMask[64];

void initTables();


}
