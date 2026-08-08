#include "evaluate.h"
#include "tables.h"
#include "../attacks/attacks.h"

namespace {
constexpr Bitboard FILE_A = 0x0101010101010101ULL;

Bitboard fileMask(int file) {
    return FILE_A << file;
}

// Pawns of color that defend sq.  The reverse pawn attack table makes this
// both clearer and faster than scanning every pawn individually.
bool pawnDefends(Square sq, Color color, Bitboard pawns) {
    Bitboard defenders = color == Color::White
        ? Attacks::blackPawnAttacks[static_cast<int>(sq)]
        : Attacks::whitePawnAttacks[static_cast<int>(sq)];
    return (defenders & pawns) != 0;
}

int kingAttackUnits(const Board& board, Square king, Color attacker) {
    const Bitboard occupied = board.getOccupied();
    const Bitboard zone = Attacks::kingAttacks[static_cast<int>(king)] | (1ULL << static_cast<int>(king));
    const Piece knight = attacker == Color::White ? Piece::WhiteKnight : Piece::BlackKnight;
    const Piece bishop = attacker == Color::White ? Piece::WhiteBishop : Piece::BlackBishop;
    const Piece rook   = attacker == Color::White ? Piece::WhiteRook   : Piece::BlackRook;
    const Piece queen  = attacker == Color::White ? Piece::WhiteQueen  : Piece::BlackQueen;

    int units = 0;
    Bitboard pieces = board.getPieces(knight);
    while (pieces) {
        int sq = BitboardOps::popLSB(pieces);
        if (Attacks::knightAttacks[sq] & zone) units += 2;
    }
    pieces = board.getPieces(bishop);
    while (pieces) {
        int sq = BitboardOps::popLSB(pieces);
        if (Attacks::bishopAttacksBB(static_cast<Square>(sq), occupied) & zone) units += 2;
    }
    pieces = board.getPieces(rook);
    while (pieces) {
        int sq = BitboardOps::popLSB(pieces);
        if (Attacks::rookAttacksBB(static_cast<Square>(sq), occupied) & zone) units += 3;
    }
    pieces = board.getPieces(queen);
    while (pieces) {
        int sq = BitboardOps::popLSB(pieces);
        if (Attacks::queenAttacksBB(static_cast<Square>(sq), occupied) & zone) units += 5;
    }
    return units;
}
}

int Evaluate::evaluatePieceSquareTables(const Board& board, int& gamePhase) {
    int mgScore = 0;
    int egScore = 0;
    gamePhase = 0;

    for (int p = static_cast<int>(Piece::WhitePawn); p <= static_cast<int>(Piece::BlackKing); p++) {
        Piece piece = static_cast<Piece>(p);
        Bitboard pieces = board.getPieces(piece);
        int sign = (pieceColor(piece) == Color::White) ? 1 : -1;

        while (pieces) {
            int sq = BitboardOps::popLSB(pieces);
            mgScore += sign * Tables::mgTable[p][sq];
            egScore += sign * Tables::egTable[p][sq];
            gamePhase += Tables::gamePhaseInc[p];
        }
    }

    if (gamePhase > 24) gamePhase = 24;
    int mgPhase = gamePhase;
    int egPhase = 24 - gamePhase;
    return (mgScore * mgPhase + egScore * egPhase) / 24;
}

int Evaluate::evaluateBishopPair(const Board& board) {
    int score = 0;
    if (BitboardOps::countBits(board.getPieces(Piece::WhiteBishop)) >= 2) score += 30;
    if (BitboardOps::countBits(board.getPieces(Piece::BlackBishop)) >= 2) score -= 30;
    return score;
}

int Evaluate::evaluateRooks(const Board& board) {
    int score = 0;
    Bitboard whitePawns = board.getPieces(Piece::WhitePawn);
    Bitboard blackPawns = board.getPieces(Piece::BlackPawn);
    Bitboard whiteRooks = board.getPieces(Piece::WhiteRook);
    Bitboard blackRooks = board.getPieces(Piece::BlackRook);

    // 7th rank bonus
    score += BitboardOps::countBits(whiteRooks & 0x00FF000000000000ULL) * 40;
    score -= BitboardOps::countBits(blackRooks & 0x000000000000FF00ULL) * 40;

    // Open / semi-open file
    for (int file = 0; file < 8; file++) {
        Bitboard mask = fileMask(file);
        bool wPawn = (whitePawns & mask) != 0;
        bool bPawn = (blackPawns & mask) != 0;
        bool wRook = (whiteRooks & mask) != 0;
        bool bRook = (blackRooks & mask) != 0;

        if (!wPawn && !bPawn) {
            if (wRook) score += 25;
            if (bRook) score -= 25;
        } else if (!wPawn && bRook) {
            score -= 15;
        } else if (!bPawn && wRook) {
            score += 15;
        }
    }

    // A rook belongs behind a passed pawn: it supports its own passer from
    // behind and restrains an enemy one from in front.
    Bitboard passers = whitePawns;
    while (passers) {
        int pawnSq = BitboardOps::popLSB(passers);
        if (blackPawns & Tables::whitePassedMask[pawnSq]) continue;
        Bitboard behind = fileMask(pawnSq % 8) & ((1ULL << pawnSq) - 1);
        if (whiteRooks & behind) score += 18;
        if (blackRooks & ~behind & fileMask(pawnSq % 8)) score -= 12;
    }
    passers = blackPawns;
    while (passers) {
        int pawnSq = BitboardOps::popLSB(passers);
        if (whitePawns & Tables::blackPassedMask[pawnSq]) continue;
        Bitboard behind = fileMask(pawnSq % 8) & ~((1ULL << (pawnSq + 1)) - 1);
        if (blackRooks & behind) score -= 18;
        if (whiteRooks & ~behind & fileMask(pawnSq % 8)) score += 12;
    }
    return score;
}

int Evaluate::evaluatePawnStructure(const Board& board) {
    int score = 0;
    Bitboard whitePawns = board.getPieces(Piece::WhitePawn);
    Bitboard blackPawns = board.getPieces(Piece::BlackPawn);

    for (int file = 0; file < 8; file++) {
        Bitboard mask = fileMask(file);
        int wCount = BitboardOps::countBits(whitePawns & mask);
        int bCount = BitboardOps::countBits(blackPawns & mask);

        if (wCount > 1) score -= (wCount - 1) * 15;
        if (bCount > 1) score += (bCount - 1) * 15;

        Bitboard adjacentFiles = 0ULL;
        if (file > 0) adjacentFiles |= 0x0101010101010101ULL << (file - 1);
        if (file < 7) adjacentFiles |= 0x0101010101010101ULL << (file + 1);

        // Isolation is a property of every pawn, not merely every file.
        if (wCount > 0 && !(whitePawns & adjacentFiles)) score -= wCount * 10;
        if (bCount > 0 && !(blackPawns & adjacentFiles)) score += bCount * 10;
    }

    // Passed pawns
    static const int passedBonus[8] = {0, 8, 18, 35, 60, 95, 145, 0};

    Bitboard tempWp = whitePawns;
    while (tempWp) {
        int sq = BitboardOps::popLSB(tempWp);
        if (!(blackPawns & Tables::whitePassedMask[sq])) {
            int rank = sq / 8;
            score += passedBonus[rank];
            Square pawnSq = static_cast<Square>(sq);
            if (pawnDefends(pawnSq, Color::White, whitePawns)) score += 12;
            Bitboard adjacent = ((sq % 8) ? fileMask(sq % 8 - 1) : 0ULL) |
                                ((sq % 8 < 7) ? fileMask(sq % 8 + 1) : 0ULL);
            if (whitePawns & adjacent) score += 8;
            if (rank < 7 && board.pieceAt(static_cast<Square>(sq + 8)) != Piece::None) score -= passedBonus[rank] / 3;
        }
    }

    Bitboard tempBp = blackPawns;
    while (tempBp) {
        int sq = BitboardOps::popLSB(tempBp);
        if (!(whitePawns & Tables::blackPassedMask[sq])) {
            int rank = sq / 8;
            int advance = 7 - rank;
            score -= passedBonus[advance];
            Square pawnSq = static_cast<Square>(sq);
            if (pawnDefends(pawnSq, Color::Black, blackPawns)) score -= 12;
            Bitboard adjacent = ((sq % 8) ? fileMask(sq % 8 - 1) : 0ULL) |
                                ((sq % 8 < 7) ? fileMask(sq % 8 + 1) : 0ULL);
            if (blackPawns & adjacent) score -= 8;
            if (rank > 0 && board.pieceAt(static_cast<Square>(sq - 8)) != Piece::None) score += passedBonus[advance] / 3;
        }
    }

    return score;
}

int Evaluate::evaluateKingSafety(const Board& board, int gamePhase) {
    int score = 0;
    Bitboard whiteKing = board.getPieces(Piece::WhiteKing);
    Bitboard blackKing = board.getPieces(Piece::BlackKing);
    if (!whiteKing || !blackKing) return 0;

    Square wk = static_cast<Square>(BitboardOps::getLSB(whiteKing));
    Square bk = static_cast<Square>(BitboardOps::getLSB(blackKing));

    auto pawnShield = [&](Square king, Color color) {
        int penalty = 0;
        int sq = static_cast<int>(king);
        int rank = sq / 8;
        int file = sq % 8;
        for (int df = -1; df <= 1; df++) {
            int f = file + df;
            if (f < 0 || f > 7) continue;
            int frontRank = (color == Color::White) ? rank + 1 : rank - 1;
            int rearRank = (color == Color::White) ? rank + 2 : rank - 2;
            Piece wanted = color == Color::White ? Piece::WhitePawn : Piece::BlackPawn;
            if (frontRank < 0 || frontRank > 7 || board.pieceAt(static_cast<Square>(frontRank * 8 + f)) != wanted)
                penalty += 12;
            else if (rearRank >= 0 && rearRank < 8 && board.pieceAt(static_cast<Square>(rearRank * 8 + f)) != wanted)
                penalty += 3;

            Bitboard mask = fileMask(f);
            Bitboard ownPawns = color == Color::White ? board.getPieces(Piece::WhitePawn) : board.getPieces(Piece::BlackPawn);
            Bitboard enemyPawns = color == Color::White ? board.getPieces(Piece::BlackPawn) : board.getPieces(Piece::WhitePawn);
            if (!(ownPawns & mask)) penalty += 10;
            if (!(ownPawns & mask) && !(enemyPawns & mask)) penalty += 8;
        }
        return penalty;
    };

    int whitePenalty = pawnShield(wk, Color::White) + kingAttackUnits(board, wk, Color::Black) * 4;
    int blackPenalty = pawnShield(bk, Color::Black) + kingAttackUnits(board, bk, Color::White) * 4;

    // King exposure matters far less after the heavy pieces have left.
    score += ((blackPenalty - whitePenalty) * gamePhase) / 24;
    return score;
}

int Evaluate::evaluateMobility(const Board& board) {
    int score = 0;
    Bitboard occ = board.getOccupied();
    Bitboard wPieces = board.getWhitePieces();
    Bitboard bPieces = board.getBlackPieces();

    // Knight mobility
    Bitboard tmp = board.getPieces(Piece::WhiteKnight);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score += (BitboardOps::countBits(Attacks::knightAttacks[sq] & ~wPieces) - 4) * 4;
    }
    tmp = board.getPieces(Piece::BlackKnight);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score -= (BitboardOps::countBits(Attacks::knightAttacks[sq] & ~bPieces) - 4) * 4;
    }

    // Bishop mobility
    tmp = board.getPieces(Piece::WhiteBishop);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score += (BitboardOps::countBits(Attacks::bishopAttacksBB(static_cast<Square>(sq), occ) & ~wPieces) - 7) * 3;
    }
    tmp = board.getPieces(Piece::BlackBishop);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score -= (BitboardOps::countBits(Attacks::bishopAttacksBB(static_cast<Square>(sq), occ) & ~bPieces) - 7) * 3;
    }

    // Rook mobility
    tmp = board.getPieces(Piece::WhiteRook);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score += (BitboardOps::countBits(Attacks::rookAttacksBB(static_cast<Square>(sq), occ) & ~wPieces) - 7) * 2;
    }
    tmp = board.getPieces(Piece::BlackRook);
    while (tmp) {
        int sq = BitboardOps::popLSB(tmp);
        score -= (BitboardOps::countBits(Attacks::rookAttacksBB(static_cast<Square>(sq), occ) & ~bPieces) - 7) * 2;
    }

    return score;
}

int Evaluate::evaluateMinorPieceStructure(const Board& board) {
    int score = 0;
    const Bitboard whitePawns = board.getPieces(Piece::WhitePawn);
    const Bitboard blackPawns = board.getPieces(Piece::BlackPawn);

    // A supported knight which cannot be immediately chased by a pawn is a
    // durable outpost.  Squares short of the fourth rank are not rewarded.
    auto outposts = [&](Piece knight, Color color, Bitboard friendlyPawns, Bitboard enemyPawns) {
        int bonus = 0;
        Bitboard knights = board.getPieces(knight);
        while (knights) {
            int sq = BitboardOps::popLSB(knights);
            int rank = sq / 8;
            if ((color == Color::White ? rank >= 3 : rank <= 4) &&
                pawnDefends(static_cast<Square>(sq), color, friendlyPawns) &&
                !pawnDefends(static_cast<Square>(sq), color == Color::White ? Color::Black : Color::White, enemyPawns)) {
                bonus += 18;
            }
        }
        return bonus;
    };
    score += outposts(Piece::WhiteKnight, Color::White, whitePawns, blackPawns);
    score -= outposts(Piece::BlackKnight, Color::Black, blackPawns, whitePawns);

    // Same-color pawn chains restrict a bishop's long-term scope.  The cap
    // avoids double-counting the immediate blockage already in mobility.
    auto badBishops = [&](Piece bishop, Bitboard pawns) {
        int count = 0;
        Bitboard bishops = board.getPieces(bishop);
        while (bishops) {
            int bishopSq = BitboardOps::popLSB(bishops);
            const bool darkSquare = ((bishopSq / 8) + (bishopSq % 8)) % 2 != 0;
            Bitboard copy = pawns;
            while (copy) {
                int pawnSq = BitboardOps::popLSB(copy);
                if ((((pawnSq / 8) + (pawnSq % 8)) % 2 != 0) == darkSquare) ++count;
            }
        }
        return std::min(count * 3, 24);
    };
    score -= badBishops(Piece::WhiteBishop, whitePawns);
    score += badBishops(Piece::BlackBishop, blackPawns);
    return score;
}

int Evaluate::evaluateMaterial(const Board& board) {
    int score = 0;
    for (int p = 0; p < 12; p++) {
        int sign = (p < 6) ? 1 : -1;
        score += sign * BitboardOps::countBits(board.getPieces(static_cast<Piece>(p + 1))) * PieceValue[p];
    }
    return score;
}

int Evaluate::evaluateTempo(const Board& board) {
    return (board.getSideToMove() == Color::White) ? 10 : -10;
}

int Evaluate::evaluate(const Board& board) {
    int score = 0;
    int gamePhase = 0;

    score += evaluatePieceSquareTables(board, gamePhase);
    score += evaluatePawnStructure(board);
    score += evaluateBishopPair(board);
    score += evaluateRooks(board);
    score += evaluateKingSafety(board, gamePhase);
    score += evaluateMobility(board);
    score += evaluateMinorPieceStructure(board);
    score += evaluateTempo(board);

    return (board.getSideToMove() == Color::White) ? score : -score;
}
