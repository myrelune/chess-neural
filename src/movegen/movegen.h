#pragma once

#include "../board/board.h"
#include "../board/types.h"
#include "move.h"

namespace MoveGen {
    MoveList generateMoves(const Board& board);
    MoveList generateLegalMoves(Board& board);

    void generateCaptureMoves(const Board& board, MoveList& moveList);
}
