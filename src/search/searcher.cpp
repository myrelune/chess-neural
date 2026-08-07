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

int Searcher::scoreMove(const Board& board, Move move, Move ttMove) {
    // 1. Highest priority: The best move found in the Transposition Table
    if (move == ttMove) {
        return 30000;
    }

    // 2. Second priority: Move that was best in the previous iterative deepening iteration
    if (move == bestMoveFound) {
        return 20000;
    }

    // 3. Captures: MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    Piece target = board.pieceAt(move.getToSquare());
    if (target != Piece::None) {
        Piece attacker = board.pieceAt(move.getFromSquare());
        return 10000 + getPieceValue(target) - (getPieceValue(attacker) / 10);
    }

    return 0; // Quiet moves scored lower
}

void Searcher::orderMoves(const Board& board, MoveList& moves, Move ttMove) {
    struct ScoredMove {
        Move move;
        int score;
    };

    ScoredMove scoredMoves[256];
    for (int i = 0; i < moves.count; i++) {
        scoredMoves[i] = { moves.moves[i], scoreMove(board, moves.moves[i], ttMove) };
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

    uint64_t key = board.getZobristKey();
    int originalAlpha = alpha;

    // PROBE TRANSPOSITION TABLE
    Move ttBestMove = Move();
    int ttScore = 0;
    if (depth > 0 && tt.probe(key, depth, alpha, beta, ttScore, ttBestMove)) {
        return ttScore; // Cache hit, Skip subtree search.
    }

    if (depth <= 0) {
        return quiescence(board, alpha, beta, info);
    }

    Color side = board.getSideToMove();
    Square kingSq = static_cast<Square>(BitboardOps::getLSB(
        board.getPieces(side == Color::White ? Piece::WhiteKing : Piece::BlackKing)
    ));

    bool inCheck = board.isSquareAttacked(kingSq, side == Color::White ? Color::Black : Color::White);

    if (depth >= 3 && !inCheck) {
        Bitboard nonPawnPieces = board.getPieces(side) & ~(board.getPieces(side == Color::White ? Piece::WhitePawn : Piece::BlackPawn) | board.getPieces(side == Color::White ? Piece::WhiteKing : Piece::BlackKing));
        
        if (nonPawnPieces != 0) {
            Undo nullUndo;
            board.makeNullMove(nullUndo);

            int R = 2;
            int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, info);

            board.unmakeNullMove(nullUndo);

            if (info.stopped) return 0;

            if (score >= beta) {
                return beta; // Fail high, prune this subtree
            }
        }
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

    // Pass the cached TT best move to prioritize it during move ordering
    orderMoves(board, moves, ttBestMove);

    int bestScore = -INFINITY_SCORE;
    Move currentBestMove = moves.moves[0]; // Fallback

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        Undo undo;
        if (!board.makeMove(move, undo)) continue;

        int score = -negamax(board, depth - 1, -beta, -alpha, info);
        board.unmakeMove(move, undo);

        if (info.stopped) return 0;

        if (score > bestScore) {
            bestScore = score;
            currentBestMove = move;
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            break; // Beta cutoff (Fail-high)
        }
    }

    // STORE RESULT IN TRANSPOSITION TABLE BEFORE RETURNING
    Bound bound = Bound::None;
    if (bestScore <= originalAlpha) {
        bound = Bound::Upper; // Fail low (didn't beat alpha)
    } else if (bestScore >= beta) {
        bound = Bound::Lower; // Fail high (beta cutoff)
    } else {
        bound = Bound::Exact; // PV node (exact score)
    }

    tt.store(key, bestScore, depth, bound, currentBestMove);

    return bestScore;
}

Move Searcher::findBestMove(Board& board, const SearchLimits& limits) {
    SearchInfo info;
    info.reset(limits.moveTimeMs);

    MoveList moves = MoveGen::generateLegalMoves(board);
    if (moves.count == 0) return Move(); // Return empty move if no legal moves exist

    // Absolute safety fallback: always default to the first legal move generated
    bestMoveFound = moves.moves[0]; 

    for (int currentDepth = 1; currentDepth <= limits.maxDepth; currentDepth++) {
        Move bestMoveThisDepth = moves.moves[0]; // Default to first legal move
        int bestScoreThisDepth = -INFINITY_SCORE;
        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;

        orderMoves(board, moves, bestMoveFound);

        for (int i = 0; i < moves.count; i++) {
            Move move = moves.moves[i];
            Undo undo;

            if (!board.makeMove(move, undo)) continue;
            int score = -negamax(board, currentDepth - 1, -beta, -alpha, info);
            board.unmakeMove(move, undo);

            if (info.stopped) break; // Discard partial iteration on timeout

            if (i == 0 || score > bestScoreThisDepth) {
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