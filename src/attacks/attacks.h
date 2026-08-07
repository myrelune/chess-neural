#pragma once

#include "../board/board.h"
#include "../board/bitboard.h"

namespace Attacks {
    extern Bitboard knightAttacks[64];
    extern Bitboard kingAttacks[64];
    extern Bitboard queenAttacks[64];
    extern Bitboard whitePawnAttacks[64];
    extern Bitboard blackPawnAttacks[64];

    extern Bitboard bishopAttacks[64][512];
    extern Bitboard rookAttacks[64][4096];

    extern uint64_t rookMagics[64];
    extern uint64_t bishopMagics[64];
    extern int rookShifts[64];
    extern int bishopShifts[64];
    extern Bitboard rookMasks[64];
    extern Bitboard bishopMasks[64];

    void initAttacks();

    inline Bitboard knightAttacksBB(Square s) {
        return knightAttacks[static_cast<int>(s)]; 
    }

    inline Bitboard kingAttacksBB(Square s) { 
        return kingAttacks[static_cast<int>(s)]; 
    }

    inline Bitboard pawnAttacksBB(Square s, Color c) {
        return c == Color::White ? whitePawnAttacks[static_cast<int>(s)]
                                 : blackPawnAttacks[static_cast<int>(s)];
    }

    Bitboard rookAttacksBB(Square s, Bitboard occupied);
    Bitboard bishopAttacksBB(Square s, Bitboard occupied);
    inline Bitboard queenAttacksBB(Square s, Bitboard occupied) {
        return rookAttacksBB(s, occupied) | bishopAttacksBB(s, occupied);
    }

    bool isSquareAttacked(const Board& board, Square square, Color attackerColor);
}

