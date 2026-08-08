#include "evaluate.h"
#include "tables.h"
#include "../attacks/attacks.h"

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
        Bitboard fileMask = 0x0101010101010101ULL << file;
        bool wPawn = (whitePawns & fileMask) != 0;
        bool bPawn = (blackPawns & fileMask) != 0;
        bool wRook = (whiteRooks & fileMask) != 0;
        bool bRook = (blackRooks & fileMask) != 0;

        if (!wPawn && !bPawn) {
            if (wRook) score += 25;
            if (bRook) score -= 25;
        } else if (!wPawn && bRook) {
            score -= 15;
        } else if (!bPawn && wRook) {
            score += 15;
        }
    }
    return score;
}

int Evaluate::evaluatePawnStructure(const Board& board) {
    int score = 0;
    Bitboard whitePawns = board.getPieces(Piece::WhitePawn);
    Bitboard blackPawns = board.getPieces(Piece::BlackPawn);

    for (int file = 0; file < 8; file++) {
        Bitboard fileMask = 0x0101010101010101ULL << file;
        int wCount = BitboardOps::countBits(whitePawns & fileMask);
        int bCount = BitboardOps::countBits(blackPawns & fileMask);

        if (wCount > 1) score -= (wCount - 1) * 15;
        if (bCount > 1) score += (bCount - 1) * 15;

        Bitboard adjacentFiles = 0ULL;
        if (file > 0) adjacentFiles |= 0x0101010101010101ULL << (file - 1);
        if (file < 7) adjacentFiles |= 0x0101010101010101ULL << (file + 1);

        if (wCount > 0 && !(whitePawns & adjacentFiles)) score -= 10;
        if (bCount > 0 && !(blackPawns & adjacentFiles)) score += 10;
    }

    // Passed pawns
    static const int passedBonus[8] = {0, 10, 20, 35, 55, 80, 110, 0};

    Bitboard tempWp = whitePawns;
    while (tempWp) {
        int sq = BitboardOps::popLSB(tempWp);
        if (!(blackPawns & Tables::whitePassedMask[sq])) score += passedBonus[sq / 8];
    }

    Bitboard tempBp = blackPawns;
    while (tempBp) {
        int sq = BitboardOps::popLSB(tempBp);
        if (!(whitePawns & Tables::blackPassedMask[sq])) score -= passedBonus[7 - (sq / 8)];
    }

    return score;
}

int Evaluate::evaluateKingSafety(const Board& board) {
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
            int pawnRank = (color == Color::White) ? rank + 1 : rank - 1;
            if (pawnRank < 0 || pawnRank > 7) continue;
            Piece pawn = board.pieceAt(static_cast<Square>(pawnRank * 8 + f));
            if ((color == Color::White && pawn != Piece::WhitePawn) ||
                (color == Color::Black && pawn != Piece::BlackPawn)) {
                penalty += 15;
            }
        }
        return penalty;
    };

    score -= pawnShield(wk, Color::White);
    score += pawnShield(bk, Color::Black);
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
    score += evaluateKingSafety(board);
    score += evaluateMobility(board);
    score += evaluateTempo(board);

    return (board.getSideToMove() == Color::White) ? score : -score;
}