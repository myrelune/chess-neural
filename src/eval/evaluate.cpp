#include "evaluate.h"
#include "tables.h"

int Evaluate::evaluatePieceSquareTables(const Board& board, int& gamePhase) {
    int mgScore = 0;
    int egScore = 0;
    gamePhase = 0;

    // Iterate through all 12 piece types using bitboards
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
    int whiteBishops = BitboardOps::countBits(board.getPieces(Piece::WhiteBishop));
    int blackBishops = BitboardOps::countBits(board.getPieces(Piece::BlackBishop));

    if (whiteBishops >= 2) score += 30;
    if (blackBishops >= 2) score -= 30;

    return score;
}

int Evaluate::evaluateRooks(const Board& board) {
    int score = 0;

    Bitboard whitePawns = board.getPieces(Piece::WhitePawn);
    Bitboard blackPawns = board.getPieces(Piece::BlackPawn);
    Bitboard whiteRooks = board.getPieces(Piece::WhiteRook);
    Bitboard blackRooks = board.getPieces(Piece::BlackRook);

    // 7th Rank Bonuses (White on rank 6, Black on rank 1 - 0-indexed)
    Bitboard whiteRank7Rooks = whiteRooks & 0x00FF000000000000ULL;
    Bitboard blackRank2Rooks = blackRooks & 0x000000000000FF00ULL;
    score += BitboardOps::countBits(whiteRank7Rooks) * 40;
    score -= BitboardOps::countBits(blackRank2Rooks) * 40;

    // Open and Semi-Open File Evaluations
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

        // Doubled Pawns
        if (wCount > 1) score -= (wCount - 1) * 15;
        if (bCount > 1) score += (bCount - 1) * 15;

        // Isolated Pawns
        Bitboard adjacentFiles = 0ULL;
        if (file > 0) adjacentFiles |= 0x0101010101010101ULL << (file - 1);
        if (file < 7) adjacentFiles |= 0x0101010101010101ULL << (file + 1);

        if (wCount > 0 && !(whitePawns & adjacentFiles)) score -= 10;
        if (bCount > 0 && !(blackPawns & adjacentFiles)) score += 10;
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

int Evaluate::evaluate(const Board& board) {
    int score = 0;
    int gamePhase = 0;

    score += evaluatePieceSquareTables(board, gamePhase);
    score += evaluatePawnStructure(board);
    score += evaluateBishopPair(board);
    score += evaluateRooks(board);
    score += evaluateKingSafety(board);

    // Return score relative to side to move for Negamax
    return (board.getSideToMove() == Color::White) ? score : -score;
}