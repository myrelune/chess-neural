#include "evaluate.h"
#include "tables.h"
#include "../movegen/movegen.h"

int Evaluate::evaluatePieceSquareTables(const Board& board) {
    int mgScore = 0;
    int egScore = 0;
    int gamePhase = 0;

    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.pieceAt(static_cast<Square>(sq));

        if (piece == Piece::None)
            continue;

        int p = static_cast<int>(piece);

        int sign = (pieceColor(piece) == Color::White) ? 1 : -1;

        mgScore += sign * Tables::mgTable[p][sq];
        egScore += sign * Tables::egTable[p][sq];

        gamePhase += Tables::gamePhaseInc[p];
    }

    // Maximum phase is 24
    if (gamePhase > 24)
        gamePhase = 24;

    int mgPhase = gamePhase;
    int egPhase = 24 - gamePhase;

    int score = (mgScore * mgPhase + egScore * egPhase) / 24;

    return score;
}

int Evaluate::evaluateBishopPair(const Board& board) {
    int score = 0;

    int whiteBishops =
        BitboardOps::countBits(
            board.getPieces(Piece::WhiteBishop)
        );

    int blackBishops =
        BitboardOps::countBits(
            board.getPieces(Piece::BlackBishop)
        );

    if (whiteBishops >= 2)
        score += 30;

    if (blackBishops >= 2)
        score -= 30;

    return score;
}

int Evaluate::evaluateRooks(const Board& board) {
    int score = 0;

    for(int file = 0; file < 8; file++) {
        bool whitePawn = false;
        bool blackPawn = false;
        bool whiteRook = false;
        bool blackRook = false;

        for(int rank = 0; rank < 8; rank++) {
            Piece p =
                board.pieceAt(
                    static_cast<Square>(rank * 8 + file)
                );

            if(p == Piece::WhitePawn)
                whitePawn = true;

            if(p == Piece::BlackPawn)
                blackPawn = true;

            if(p == Piece::WhiteRook)
                whiteRook = true;

            if(p == Piece::BlackRook)
                blackRook = true;
        }

        // Open file
        if(!whitePawn && !blackPawn) {
            if(whiteRook)
                score += 25;

            if(blackRook)
                score -= 25;
        }

        // Semi-open file
        else if(!whitePawn && blackRook) {
            score -= 15;
        }

        else if(!blackPawn && whiteRook) {
            score += 15;
        }
    }

    // Bonus for rooks on the 7th rank
    for(int file = 0; file < 8; file++) {
        Piece whiteRank7 =
            board.pieceAt(
                static_cast<Square>(6 * 8 + file)
            );

        Piece blackRank2 =
            board.pieceAt(
                static_cast<Square>(1 * 8 + file)
            );

        if(whiteRank7 == Piece::WhiteRook)
            score += 40;

        if(blackRank2 == Piece::BlackRook)
            score -= 40;
    }

    return score;
}

int Evaluate::evaluatePawnStructure(const Board& board) {
    int score = 0;

    for(int file = 0; file < 8; file++) {
        int whitePawns = 0;
        int blackPawns = 0;

        for(int rank = 0; rank < 8; rank++) {
            Square sq = static_cast<Square>(rank * 8 + file);
            Piece piece = board.pieceAt(sq);

            if(piece == Piece::WhitePawn)
                whitePawns++;

            if(piece == Piece::BlackPawn)
                blackPawns++;
        }

        if(whitePawns > 1)
            score -= (whitePawns - 1) * 15;

        if(blackPawns > 1)
            score += (blackPawns - 1) * 15;

        bool whiteNeighbor = false;
        bool blackNeighbor = false;

        if(file > 0) {
            for(int rank=0; rank<8; rank++) {
                Piece p = board.pieceAt(
                    static_cast<Square>(rank*8 + file-1)
                );

                if(p == Piece::WhitePawn)
                    whiteNeighbor = true;

                if(p == Piece::BlackPawn)
                    blackNeighbor = true;
            }
        }

        if(file < 7) {
            for(int rank=0; rank<8; rank++) {
                Piece p = board.pieceAt(
                    static_cast<Square>(rank*8 + file+1)
                );

                if(p == Piece::WhitePawn)
                    whiteNeighbor = true;

                if(p == Piece::BlackPawn)
                    blackNeighbor = true;
            }
        }

        if(whitePawns && !whiteNeighbor)
            score -= 10;

        if(blackPawns && !blackNeighbor)
            score += 10;
    }

    return score;
}

int Evaluate::evaluateMobility(const Board& board) {
    int score = 0;

    Board copy = board;

    MoveList moves = MoveGen::generateLegalMoves(copy);

    int mobility = moves.count;

    if(board.getSideToMove() == Color::White)
        score += mobility * 2;
    else
        score -= mobility * 2;

    return score;
}

int Evaluate::evaluateKingSafety(const Board& board) {
    int score = 0;

    Bitboard whiteKing = board.getPieces(Piece::WhiteKing);
    Bitboard blackKing = board.getPieces(Piece::BlackKing);

    Square wk = static_cast<Square>(BitboardOps::getLSB(whiteKing));
    Square bk = static_cast<Square>(BitboardOps::getLSB(blackKing));


    auto pawnShield = [&](Square king, Color color) {
        int penalty = 0;

        int sq = static_cast<int>(king);

        int rank = sq / 8;
        int file = sq % 8;


        for(int df=-1; df<=1; df++) {
            int f = file + df;

            if(f < 0 || f > 7)
                continue;

            int pawnRank =
                (color == Color::White)
                ? rank + 1
                : rank - 1;

            if(pawnRank < 0 || pawnRank > 7)
                continue;

            Piece pawn =
                board.pieceAt(
                    static_cast<Square>(pawnRank*8+f)
                );

            if(
                (color == Color::White &&
                 pawn != Piece::WhitePawn)
                ||
                (color == Color::Black &&
                 pawn != Piece::BlackPawn)
              ) {
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
    Board copy = board;

    MoveList moves = MoveGen::generateLegalMoves(copy);

    if (moves.count == 0) {
        Square king = static_cast<Square>(
            BitboardOps::getLSB(board.getPieces(board.getSideToMove() == Color::White ? Piece::WhiteKing : Piece::BlackKing))
        );

        Color enemy = board.getSideToMove() == Color::White ? Color::Black : Color::White;

        if (board.isSquareAttacked(king, enemy)) {
            return -1000000;
        }

        return 0;
    }

    int score = 0;

    score += evaluatePieceSquareTables(board);
    score += evaluatePawnStructure(board);
    score += evaluateBishopPair(board);
    score += evaluateRooks(board);
    score += evaluateMobility(board);
    score += evaluateKingSafety(board);

    return (board.getSideToMove() == Color::White)
        ? score
        : -score;
}
