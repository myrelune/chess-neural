#include "searcher.h"
#include <algorithm>
#include <iostream>

namespace {
    // Piece values for MVV-LVA move ordering
    int getPieceValue(Piece piece) {
        switch (piece) {
            case Piece::WhitePawn:   case Piece::BlackPawn:   return 100;
            case Piece::WhiteKnight: case Piece::BlackKnight: return 320;
            case Piece::WhiteBishop: case Piece::BlackBishop: return 330;
            case Piece::WhiteRook:   case Piece::BlackRook:   return 500;
            case Piece::WhiteQueen:  case Piece::BlackQueen:  return 900;
            case Piece::WhiteKing:   case Piece::BlackKing:   return 20000;
            default: return 0;
        }
    }
}

int Searcher::scoreMove(const Board& board, Move move) {
    Piece target = board.pieceAt(move.getToSquare());
    if (target != Piece::None) {
        Piece attacker = board.pieceAt(move.getFromSquare());
        // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
        return 10000 + getPieceValue(target) - (getPieceValue(attacker) / 10);
    }

    // Prioritize the move that was best in the previous iterative deepening iteration
    if (move == bestMoveFound) {
        return 20000;
    }

    return 0; // Quiet moves scored lower
}

void Searcher::orderMoves(const Board& board, MoveList& moves) {
    struct ScoredMove {
        Move move;
        int score;
    };

    ScoredMove scoredMoves[256];
    for (int i = 0; i < moves.count; i++) {
        scoredMoves[i] = { moves.moves[i], scoreMove(board, moves.moves[i]) };
    }

    // Sort moves in descending order of score
    std::sort(scoredMoves, scoredMoves + moves.count, [](const ScoredMove& a, const ScoredMove& b) {
        return a.score > b.score;
    });

    for (int i = 0; i < moves.count; i++) {
        moves.moves[i] = scoredMoves[i].move;
    }
}

int Searcher::quiescence(Board& board, int alpha, int beta, SearchInfo& info) {
    info.nodes++;
    if (info.checkLimits()) return 0;

    int standPat = Evaluate::evaluate(board);
    if (standPat >= beta) return beta;
    if (alpha < standPat) alpha = standPat;

    MoveList moves = MoveGen::generateLegalMoves(board);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];

        // Quiescence Search: Only process captures
        if (board.pieceAt(move.getToSquare()) == Piece::None) continue;

        Undo undo;
        if (!board.makeMove(move, undo)) continue;

        int score = -quiescence(board, -beta, -alpha, info);
        board.unmakeMove(move, undo);

        if (info.stopped) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Searcher::negamax(Board& board, int depth, int alpha, int beta, SearchInfo& info) {
    info.nodes++;
    if (info.checkLimits()) return 0;

    if (depth <= 0) {
        return quiescence(board, alpha, beta, info);
    }

    MoveList moves = MoveGen::generateLegalMoves(board);

    // Terminal node handling: Checkmate or Stalemate
    if (moves.count == 0) {
        Color side = board.getSideToMove();
        Square kingSq = static_cast<Square>(BitboardOps::getLSB(
            board.getPieces(side == Color::White ? Piece::WhiteKing : Piece::BlackKing)
        ));

        if (board.isSquareAttacked(kingSq, side == Color::White ? Color::Black : Color::White)) {
            return -MATE_SCORE + (64 - depth); // Prefer shorter mate paths
        }
        return 0; // Stalemate
    }

    orderMoves(board, moves);

    for (int i = 0; i < moves.count; i++) {
        Undo undo;
        if (!board.makeMove(moves.moves[i], undo)) continue;

        int score = -negamax(board, depth - 1, -beta, -alpha, info);
        board.unmakeMove(moves.moves[i], undo);

        if (info.stopped) return 0;

        if (score >= beta) return beta; // Beta cutoff
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Move Searcher::findBestMove(Board& board, const SearchLimits& limits) {
    SearchInfo info;
    info.reset(limits.moveTimeMs);

    MoveList moves = MoveGen::generateLegalMoves(board);
    if (moves.count == 0) return Move(); // Return empty move if no legal moves exist

    bestMoveFound = moves.moves[0]; // Fallback to first move

    for (int currentDepth = 1; currentDepth <= limits.maxDepth; currentDepth++) {
        Move bestMoveThisDepth = bestMoveFound;
        int bestScoreThisDepth = -INFINITY_SCORE;
        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;

        orderMoves(board, moves);

        for (int i = 0; i < moves.count; i++) {
            Move move = moves.moves[i];
            Undo undo;

            if (!board.makeMove(move, undo)) continue;
            int score = -negamax(board, currentDepth - 1, -beta, -alpha, info);
            board.unmakeMove(move, undo);

            if (info.stopped) break; // Discard partial iteration on timeout

            if (score > bestScoreThisDepth) {
                bestScoreThisDepth = score;
                bestMoveThisDepth = move;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        // Commit best move only if the current depth finished completely
        if (!info.stopped) {
            bestMoveFound = bestMoveThisDepth;
            if (limits.maxNodes > 0 && info.nodes >= limits.maxNodes) break;
        } else {
            break;
        }
    }

    return bestMoveFound;
}