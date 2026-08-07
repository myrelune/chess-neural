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

void Searcher::clearHeuristics() {
    for (int p = 0; p < 64; p++) {
        killerMoves[p][0] = Move();
        killerMoves[p][1] = Move();
    }
    for (int c = 0; c < 2; c++) {
        for (int f = 0; f < 64; f++) {
            for (int t = 0; t < 64; t++) {
                historyTable[c][f][t] = 0;
            }
        }
    }
}

int Searcher::scoreMove(const Board& board, Move move, Move ttMove, int ply) {
    if (move == ttMove) return 30000;
    if (move == bestMoveFound) return 25000;

    Piece target = board.pieceAt(move.getToSquare());
    if (target != Piece::None) {
        Piece attacker = board.pieceAt(move.getFromSquare());
        return 20000 + getPieceValue(target) - (getPieceValue(attacker) / 10);
    }

    if (ply < 64) {
        if (move == killerMoves[ply][0]) return 15000;
        if (move == killerMoves[ply][1]) return 14000;
    }

    Color side = board.getSideToMove();
    int histScore = historyTable[static_cast<int>(side)][static_cast<int>(move.getFromSquare())][static_cast<int>(move.getToSquare())];
    
    return std::min(histScore, 10000);
}

void Searcher::orderMoves(const Board& board, MoveList& moves, Move ttMove, int ply) {
    int scores[256];

    for (int i = 0; i < moves.count; i++) {
        scores[i] = scoreMove(board, moves.moves[i], ttMove, ply);
    }

    for (int i = 0; i < moves.count - 1; i++) {
        int bestIndex = i;
        for (int j = i + 1; j < moves.count; j++) {
            if (scores[j] > scores[bestIndex]) {
                bestIndex = j;
            }
        }

        std::swap(moves.moves[i], moves.moves[bestIndex]);
        std::swap(scores[i], scores[bestIndex]);
    }
}

int Searcher::quiescence(Board& board, int alpha, int beta, SearchInfo& info) {
    info.nodes++;

    int standPat = Evaluate::evaluate(board);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    MoveList captures;
    MoveGen::generateCaptureMoves(board, captures);

    orderMoves(board, captures, Move(), 64);

    for (int i = 0; i < captures.count; i++) {
        Move move = captures.moves[i];
        Undo undo;

        if (!board.makeMove(move, undo)) continue;

        // Verify move legality (King cannot be left in check)
        Color opponentColor = board.getSideToMove();
        Color ourSide = (opponentColor == Color::White) ? Color::Black : Color::White;
        Bitboard kingBb = board.getPieces((ourSide == Color::White) ? Piece::WhiteKing : Piece::BlackKing);
        if (kingBb != 0) {
            Square kingSq = static_cast<Square>(BitboardOps::getLSB(kingBb));
            if (board.isSquareAttacked(kingSq, opponentColor)) {
                board.unmakeMove(move, undo);
                continue; // Skip illegal capture
            }
        }

        int score = -quiescence(board, -beta, -alpha, info);
        board.unmakeMove(move, undo);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Searcher::negamax(Board& board, int depth, int alpha, int beta, SearchInfo& info, int ply) {
    info.nodes++;
    if (info.checkLimits()) return 0;

    // Repetition / 50-move draw
    if (ply > 0 && (board.isRepetition() || board.isDraw())) return 0;

    uint64_t key = board.getZobristKey();
    int originalAlpha = alpha;

    Color side = board.getSideToMove();
    Bitboard kingBb = board.getPieces(side == Color::White ? Piece::WhiteKing : Piece::BlackKing);
    bool inCheck = false;
    if (kingBb != 0) {
        Square kingSq = static_cast<Square>(BitboardOps::getLSB(kingBb));
        inCheck = board.isSquareAttacked(kingSq, side == Color::White ? Color::Black : Color::White);
    }

    // PROBE TRANSPOSITION TABLE
    Move ttBestMove = Move();
    int ttScore = 0;
    if (depth > 0 && tt.probe(key, depth, alpha, beta, ttScore, ttBestMove)) {
        return ttScore; // Cache hit, Skip subtree search.
    }

    if (depth >= 6 && ttBestMove == Move() && !inCheck) {
        negamax(board, depth - 4, alpha, beta, info, ply);
        // Re-probe the TT using the shallow depth (depth - 4) to retrieve the move
        int dummyScore = 0;
        tt.probe(key, depth - 4, alpha, beta, dummyScore, ttBestMove);
    }

    if (depth <= 0) {
        return quiescence(board, alpha, beta, info);
    }

    if (depth >= 3 && !inCheck) {
        Bitboard nonPawnPieces = board.getPieces(side) & ~(board.getPieces(side == Color::White ? Piece::WhitePawn : Piece::BlackPawn) | board.getPieces(side == Color::White ? Piece::WhiteKing : Piece::BlackKing));

        if (nonPawnPieces != 0) {
            Undo nullUndo;
            board.makeNullMove(nullUndo);

            int R = 3 + depth / 4;
            int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, info, ply + 1);

            board.unmakeNullMove(nullUndo);

            if (info.stopped) return 0;

            if (score >= beta) {
                return beta; // Fail high, prune this subtree
            }
        }
    }

    MoveList moves = MoveGen::generateMoves(board);

    // Pass the cached TT best move to prioritize it during move ordering
    orderMoves(board, moves, ttBestMove, ply);

    int bestScore = -INFINITY_SCORE;
    Move currentBestMove = Move();
    int legalMovesCount = 0;

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        Undo undo;
        if (!board.makeMove(move, undo)) continue;

        // Check if move was legal (did it leave our king in check?)
        Color opponentColor = board.getSideToMove();
        Color ourSide = (opponentColor == Color::White) ? Color::Black : Color::White;
        Bitboard kingBitboard = board.getPieces((ourSide == Color::White) ? Piece::WhiteKing : Piece::BlackKing);
        if (kingBitboard != 0) {
            Square kingSquare = static_cast<Square>(BitboardOps::getLSB(kingBitboard));
            if (board.isSquareAttacked(kingSquare, opponentColor)) {
                board.unmakeMove(move, undo);
                continue; // Illegal move
            }
        }

        legalMovesCount++;

        bool isCapture = board.pieceAt(move.getToSquare()) != Piece::None;
        bool givesCheck = false; // approximate - skip for now

        int score = 0;
        if (legalMovesCount == 1) {
            // Principal Variation Search: Full window search for 1st move
            score = -negamax(board, depth - 1, -beta, -alpha, info, ply + 1);
        } else {
            // Late Move Reduction: logarithmic reduction based on depth and move index
            int reduction = 0;
            if (legalMovesCount >= 4 && depth >= 3 && !isCapture && !inCheck && !givesCheck
                && move.promotion == Piece::None) {

                // Standard logarithmic LMR approximation formula
                double depthLog = std::log(static_cast<double>(depth));
                double moveCountLog = std::log(static_cast<double>(legalMovesCount));
                reduction = static_cast<int>(0.5 + depthLog * moveCountLog / 2.0);

                // Clamp reduction bounds for safety
                if (reduction < 1) reduction = 1;
                if (reduction > depth - 2) reduction = depth - 2;
            }

            // Zero-window search with reduction
            score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, info, ply + 1);

            // Re-search full depth if LMR failed high
            if (score > alpha && reduction > 0) {
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, info, ply + 1);
            }

            // Re-search with full window if zero-window failed high
            if (score > alpha && score < beta) {
                score = -negamax(board, depth - 1, -beta, -alpha, info, ply + 1);
            }
        }

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
            // Store Killer move & History score for quiet moves causing beta cutoff
            if (board.pieceAt(move.getToSquare()) == Piece::None && ply < 64) {
                killerMoves[ply][1] = killerMoves[ply][0];
                killerMoves[ply][0] = move;
                historyTable[static_cast<int>(ourSide)][static_cast<int>(move.getFromSquare())][static_cast<int>(move.getToSquare())] += depth * depth;
            }
            break; // Beta cutoff (Fail-high)
        }
    }

    // Terminal node handling: Checkmate or Stalemate
    if (legalMovesCount == 0) {
        if (inCheck) {
            return -MATE_SCORE + ply; // Prefer shorter mate paths from root
        }
        return 0; // Stalemate
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
    clearHeuristics();

    MoveList moves = MoveGen::generateLegalMoves(board);
    if (moves.count == 0) return Move(); // Return empty move if no legal moves exist

    bestMoveFound = moves.moves[0];
    int bestScoreThisDepth = 0;

    for (int currentDepth = 1; currentDepth <= limits.maxDepth; currentDepth++) {
        Move bestMoveThisDepth = moves.moves[0];
        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;

        // Aspiration Windows: Use a tight window around the previous depth's score for depth >= 5
        if (currentDepth >= 5) {
            int window = 50; // 50 centipawns window
            alpha = std::max(-INFINITY_SCORE, bestScoreThisDepth - window);
            beta = std::min(INFINITY_SCORE, bestScoreThisDepth + window);
        }

        int failedWindowRetries = 0;
        while (true) {
            orderMoves(board, moves, bestMoveFound, 0);

            int localBestScore = -INFINITY_SCORE;
            Move localBestMove = bestMoveThisDepth;

            for (int i = 0; i < moves.count; i++) {
                Move move = moves.moves[i];
                Undo undo;

                if (!board.makeMove(move, undo)) continue;

                int score = 0;
                if (i == 0) {
                    score = -negamax(board, currentDepth - 1, -beta, -alpha, info, 1);
                } else {
                    score = -negamax(board, currentDepth - 1, -alpha - 1, -alpha, info, 1);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, currentDepth - 1, -beta, -alpha, info, 1);
                    }
                }

                board.unmakeMove(move, undo);

                if (info.stopped) break; // Discard partial iteration on timeout

                if (i == 0 || score > localBestScore) {
                    localBestScore = score;
                    localBestMove = move;
                }
                if (score > alpha) {
                    alpha = score;
                }

                // Break early if we hit beta (fail-high) during aspiration window search
                if (alpha >= beta) break;
            }

            if (info.stopped) break;

            // Handle aspiration window fail low / fail high
            if (currentDepth >= 5 && (localBestScore <= alpha || localBestScore >= beta)) {
                failedWindowRetries++;
                if (localBestScore <= alpha) {
                    // Fail low: widen alpha down to negative infinity, keep beta
                    alpha = -INFINITY_SCORE;
                } else {
                    // Fail high: widen beta up to positive infinity, keep alpha
                    beta = INFINITY_SCORE;
                }

                // If we retry too many times, fallback to full window search to avoid performance loss
                if (failedWindowRetries > 2) {
                    alpha = -INFINITY_SCORE;
                    beta = INFINITY_SCORE;
                }
                continue; // Re-search current depth with widened window
            }

            bestScoreThisDepth = localBestScore;
            bestMoveThisDepth = localBestMove;
            break; // Window search successful, exit retry loop
        }

        // Commit best move only if the current depth finished completely
        if (!info.stopped) {
            bestMoveFound = bestMoveThisDepth;

            auto now = std::chrono::high_resolution_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.startTime).count();
            uint64_t nps = elapsedMs > 0 ? (info.nodes * 1000) / elapsedMs : info.nodes;

            std::cout << "info depth " << currentDepth
                      << " score cp " << bestScoreThisDepth
                      << " nodes " << info.nodes
                      << " time " << elapsedMs
                      << " nps " << nps
                      << " pv " << moveToUci(bestMoveFound)
                      << "\n" << std::flush;

            if (limits.maxNodes > 0 && info.nodes >= limits.maxNodes) break;
        } else {
            break;
        }
    }

    return bestMoveFound;
}
