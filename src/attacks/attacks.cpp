#include "attacks.h"
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>

namespace Attacks {

    // Define the global storage arrays declared in the header
    Bitboard knightAttacks[64];
    Bitboard kingAttacks[64];
    Bitboard queenAttacks[64];
    Bitboard whitePawnAttacks[64];
    Bitboard blackPawnAttacks[64];

    Bitboard bishopAttacks[64][512];
    Bitboard rookAttacks[64][4096];

    uint64_t rookMagics[64];
    uint64_t bishopMagics[64];
    int rookShifts[64];
    int bishopShifts[64];
    Bitboard rookMasks[64];
    Bitboard bishopMasks[64];

    // Helper to check board boundaries
    inline bool isValidSquare(int rank, int file) {
        return rank >= 0 && rank < 8 && file >= 0 && file < 8;
    }

    // Non Sliding Pieces Initialization
    void initLeapers() {
        for (int sq = 0; sq < 64; ++sq) {
            int rank = sq / 8;
            int file = sq % 8;

            // Knights
            int knightMoves[8][2] = {
                {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                {1, -2},  {1, 2},  {2, -1},  {2, 1}
            };
            for (auto& m : knightMoves) {
                int r = rank + m[0];
                int f = file + m[1];
                if (isValidSquare(r, f)) {
                    knightAttacks[sq] |= (1ULL << (r * 8 + f));
                }
            }

            // Kings
            int kingMoves[8][2] = {
                {-1, -1}, {-1, 0}, {-1, 1},
                {0, -1},           {0, 1},
                {1, -1},  {1, 0},  {1, 1}
            };
            for (auto& m : kingMoves) {
                int r = rank + m[0];
                int f = file + m[1];
                if (isValidSquare(r, f)) {
                    kingAttacks[sq] |= (1ULL << (r * 8 + f));
                }
            }

            // White Pawns
            if (rank < 7) {
                if (file > 0) whitePawnAttacks[sq] |= (1ULL << ((rank + 1) * 8 + (file - 1)));
                if (file < 7) whitePawnAttacks[sq] |= (1ULL << ((rank + 1) * 8 + (file + 1)));
            }

            // Black Pawns
            if (rank > 0) {
                if (file > 0) blackPawnAttacks[sq] |= (1ULL << ((rank - 1) * 8 + (file - 1)));
                if (file < 7) blackPawnAttacks[sq] |= (1ULL << ((rank - 1) * 8 + (file + 1)));
            }
        }
    }

    // Sliding Pieces: Masks & On-The-Fly Ray Generators
    Bitboard maskRook(int sq) {
        Bitboard mask = 0ULL;
        int r = sq / 8;
        int f = sq % 8;
        for (int tr = r + 1; tr <= 6; ++tr) mask |= (1ULL << (tr * 8 + f));
        for (int tr = r - 1; tr >= 1; --tr) mask |= (1ULL << (tr * 8 + f));
        for (int tf = f + 1; tf <= 6; ++tf) mask |= (1ULL << (r * 8 + tf));
        for (int tf = f - 1; tf >= 1; --tf) mask |= (1ULL << (r * 8 + tf));
        return mask;
    }

    Bitboard maskBishop(int sq) {
        Bitboard mask = 0ULL;
        int r = sq / 8;
        int f = sq % 8;
        for (int tr = r + 1, tf = f + 1; tr <= 6 && tf <= 6; ++tr, ++tf) mask |= (1ULL << (tr * 8 + tf));
        for (int tr = r + 1, tf = f - 1; tr <= 6 && tf >= 1; ++tr, --tf) mask |= (1ULL << (tr * 8 + tf));
        for (int tr = r - 1, tf = f + 1; tr >= 1 && tf <= 6; --tr, ++tf) mask |= (1ULL << (tr * 8 + tf));
        for (int tr = r - 1, tf = f - 1; tr >= 1 && tf >= 1; --tr, --tf) mask |= (1ULL << (tr * 8 + tf));
        return mask;
    }

    Bitboard rookAttacksOnTheFly(int sq, Bitboard blocked) {
        Bitboard attacks = 0ULL;
        int r = sq / 8;
        int f = sq % 8;
        for (int tr = r + 1; tr <= 7; ++tr) { attacks |= (1ULL << (tr * 8 + f)); if (blocked & (1ULL << (tr * 8 + f))) break; }
        for (int tr = r - 1; tr >= 0; --tr) { attacks |= (1ULL << (tr * 8 + f)); if (blocked & (1ULL << (tr * 8 + f))) break; }
        for (int tf = f + 1; tf <= 7; ++tf) { attacks |= (1ULL << (r * 8 + tf)); if (blocked & (1ULL << (r * 8 + tf))) break; }
        for (int tf = f - 1; tf >= 0; --tf) { attacks |= (1ULL << (r * 8 + tf)); if (blocked & (1ULL << (r * 8 + tf))) break; }
        return attacks;
    }

    Bitboard bishopAttacksOnTheFly(int sq, Bitboard blocked) {
        Bitboard attacks = 0ULL;
        int r = sq / 8;
        int f = sq % 8;
        for (int tr = r + 1, tf = f + 1; tr <= 7 && tf <= 7; ++tr, ++tf) { attacks |= (1ULL << (tr * 8 + tf)); if (blocked & (1ULL << (tr * 8 + tf))) break; }
        for (int tr = r + 1, tf = f - 1; tr <= 7 && tf >= 0; ++tr, --tf) { attacks |= (1ULL << (tr * 8 + tf)); if (blocked & (1ULL << (tr * 8 + tf))) break; }
        for (int tr = r - 1, tf = f + 1; tr >= 0 && tf <= 7; --tr, ++tf) { attacks |= (1ULL << (tr * 8 + tf)); if (blocked & (1ULL << (tr * 8 + tf))) break; }
        for (int tr = r - 1, tf = f - 1; tr >= 0 && tf >= 0; --tr, --tf) { attacks |= (1ULL << (tr * 8 + tf)); if (blocked & (1ULL << (tr * 8 + tf))) break; }
        return attacks;
    }

    // Magic Number Search & Generation
    uint64_t g_seed = 1070372ULL;
    uint32_t random32() {
        uint32_t number = (uint32_t)(g_seed);
        g_seed ^= (g_seed << 13);
        g_seed ^= (g_seed >> 7);
        g_seed ^= (g_seed << 17);
        return number;
    }

    uint64_t random64() {
        uint64_t n1 = (uint64_t)(random32()) & 0xFFFF;
        uint64_t n2 = (uint64_t)(random32()) & 0xFFFF;
        uint64_t n3 = (uint64_t)(random32()) & 0xFFFF;
        uint64_t n4 = (uint64_t)(random32()) & 0xFFFF;
        return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
    }

    uint64_t findMagicNumber(int sq, int bitCount, bool isRook) {
        Bitboard mask = isRook ? maskRook(sq) : maskBishop(sq);
        int n = 1 << bitCount;
        
        std::vector<Bitboard> occupancies(n);
        std::vector<Bitboard> attacks(n);

        for (int i = 0; i < n; ++i) {
            Bitboard occupied = 0ULL;
            int tempMask = mask;
            for (int b = 0; b < bitCount; ++b) {
                int lsbIndex = __builtin_ctzll(tempMask);
                tempMask &= tempMask - 1;
                if ((i >> b) & 1) occupied |= (1ULL << lsbIndex);
            }
            occupancies[i] = occupied;
            attacks[i] = isRook ? rookAttacksOnTheFly(sq, occupied) : bishopAttacksOnTheFly(sq, occupied);
        }

        std::vector<Bitboard> usedAttacks(n, 0ULL);
        int shift = 64 - bitCount;

        while (true) {
            uint64_t candidate = random64() & random64() & random64();
            
            if (__builtin_popcountll((candidate * mask) & 0xFF00000000000000ULL) < 6) continue;

            std::fill(usedAttacks.begin(), usedAttacks.end(), 0ULL);
            bool fail = false;

            for (int i = 0; i < n; ++i) {
                int index = (int)((occupancies[i] * candidate) >> shift);
                if (usedAttacks[index] == 0ULL) {
                    usedAttacks[index] = attacks[i];
                } else if (usedAttacks[index] != attacks[i]) {
                    fail = true;
                    break;
                }
            }

            if (!fail) return candidate;
        }
    }

    // Lookups via Magic Bitboards
    Bitboard rookAttacksBB(Square s, Bitboard occupied) {
        int sq = static_cast<int>(s);
        occupied &= rookMasks[sq];
        occupied *= rookMagics[sq];
        occupied >>= rookShifts[sq];
        return rookAttacks[sq][occupied];
    }

    Bitboard bishopAttacksBB(Square s, Bitboard occupied) {
        int sq = static_cast<int>(s);
        occupied &= bishopMasks[sq];
        occupied *= bishopMagics[sq];
        occupied >>= bishopShifts[sq];
        return bishopAttacks[sq][occupied];
    }

    // Attack Validation Routine 
    bool isSquareAttacked(const Board& board, Square square, Color attackerColor) {
        Bitboard occ = board.getOccupied(); 

        if (attackerColor == Color::White) {
            if (pawnAttacksBB(square, Color::Black) & board.getPieces(Piece::WhitePawn)) return true;
        } 

        if (knightAttacksBB(square) & board.getPieces(attackerColor == Color::White ? Piece::WhiteKnight : Piece::BlackKnight)) return true;
        if (kingAttacksBB(square) & board.getPieces(attackerColor == Color::White ? Piece::WhiteKing : Piece::BlackKing)) return true;
        
        Bitboard bishops = board.getPieces(attackerColor == Color::White ? Piece::WhiteBishop : Piece::BlackBishop);
        Bitboard rooks   = board.getPieces(attackerColor == Color::White ? Piece::WhiteRook : Piece::BlackRook);
        Bitboard queens  = board.getPieces(attackerColor == Color::White ? Piece::WhiteQueen : Piece::BlackQueen);

        if (bishopAttacksBB(square, occ) & (bishops | queens)) return true;
        if (rookAttacksBB(square, occ) & (rooks | queens)) return true;

        return false;
    }
    
    void initSlidingPieces(bool isRook) {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard mask = isRook ? maskRook(sq) : maskBishop(sq);
            if (isRook) rookMasks[sq] = mask;
            else bishopMasks[sq] = mask;

            int bitCount = __builtin_popcountll(mask);
            int shift = 64 - bitCount;
            
            if (isRook) rookShifts[sq] = shift;
            else bishopShifts[sq] = shift;

            uint64_t magic = findMagicNumber(sq, bitCount, isRook);
            if (isRook) rookMagics[sq] = magic;
            else bishopMagics[sq] = magic;

            int indices = 1 << bitCount;
            for (int i = 0; i < indices; ++i) {
                Bitboard occupied = 0ULL;
                int tempMask = mask;
                for (int b = 0; b < bitCount; ++b) {
                    int lsbIndex = __builtin_ctzll(tempMask);
                    tempMask &= tempMask - 1;
                    if ((i >> b) & 1) occupied |= (1ULL << lsbIndex);
                }

                Bitboard attackPattern = isRook ? rookAttacksOnTheFly(sq, occupied) : bishopAttacksOnTheFly(sq, occupied);
                int index = (int)((occupied * magic) >> shift);

                if (isRook) rookAttacks[sq][index] = attackPattern;
                else bishopAttacks[sq][index] = attackPattern;
            }
        }
    }

    void initAttacks() {
        initLeapers();
        initSlidingPieces(true);  // Automatically generates rook magics and populates tables
        initSlidingPieces(false); // Automatically generates bishop magics and populates tables
    }
}