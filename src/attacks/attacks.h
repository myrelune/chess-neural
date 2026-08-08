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

    inline Bitboard rookAttacksBB(Square s, Bitboard occupied) {
        int sq = static_cast<int>(s);
        Bitboard occ = occupied & rookMasks[sq];
        occ *= rookMagics[sq];
        occ >>= rookShifts[sq];
        return rookAttacks[sq][occ];
    }

    inline Bitboard bishopAttacksBB(Square s, Bitboard occupied) {
        int sq = static_cast<int>(s);
        Bitboard occ = occupied & bishopMasks[sq];
        occ *= bishopMagics[sq];
        occ >>= bishopShifts[sq];
        return bishopAttacks[sq][occ];
    }

    inline Bitboard queenAttacksBB(Square s, Bitboard occupied) {
        return rookAttacksBB(s, occupied) | bishopAttacksBB(s, occupied);
    }

    inline bool isSquareAttacked(const Board& board, Square square, Color attackerColor) {
        Bitboard occ = board.getOccupied();
        int sqIdx = static_cast<int>(square);

        if (attackerColor == Color::White) {
            if (blackPawnAttacks[sqIdx] & board.getPieces(Piece::WhitePawn)) return true;
            if (knightAttacks[sqIdx] & board.getPieces(Piece::WhiteKnight)) return true;
            if (kingAttacks[sqIdx] & board.getPieces(Piece::WhiteKing)) return true;
        } else {
            if (whitePawnAttacks[sqIdx] & board.getPieces(Piece::BlackPawn)) return true;
            if (knightAttacks[sqIdx] & board.getPieces(Piece::BlackKnight)) return true;
            if (kingAttacks[sqIdx] & board.getPieces(Piece::BlackKing)) return true;
        }

        Bitboard bishops = board.getPieces(attackerColor == Color::White ? Piece::WhiteBishop : Piece::BlackBishop);
        Bitboard rooks   = board.getPieces(attackerColor == Color::White ? Piece::WhiteRook   : Piece::BlackRook);
        Bitboard queens  = board.getPieces(attackerColor == Color::White ? Piece::WhiteQueen  : Piece::BlackQueen);

        if ((bishops | queens) && (bishopAttacksBB(square, occ) & (bishops | queens))) return true;
        if ((rooks | queens) && (rookAttacksBB(square, occ) & (rooks | queens))) return true;

        return false;
    }
}

