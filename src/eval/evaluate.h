#pragma once

#include "../board/board.h"

namespace Evaluate
{
    constexpr int PieceValue[12] = {
        100, // White Pawn
        320, // White Knight
        330, // White Bishop
        500, // White Rook
        900, // White Queen
        0,   // White King

        100, // Black Pawn
        320,
        330,
        500,
        900,
        0
    };

    int evaluate(const Board& board);

    int evaluateMaterial(const Board& board);
    int evaluatePieceSquareTables(const Board& board, int& gamePhase);
    int evaluatePawnStructure(const Board& board);
    int evaluateMobility(const Board& board);
    int evaluateKingSafety(const Board& board);
    int evaluateBishopPair(const Board& board);
    int evaluateRooks(const Board& board);
    int evaluateTempo(const Board& board);
}
