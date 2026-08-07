#ifndef SEARCHER_H
#define SEARCHER_H

#include "../board/board.h"
#include "../movegen/movegen.h"
#include "../eval/evaluate.h"
#include <chrono>
#include <cstdint>

constexpr int MATE_SCORE = 40000;
constexpr int INFINITY_SCORE = 50000;

struct SearchLimits {
    int maxDepth = 64;
    int moveTimeMs = 0;      // Fixed time per move (0 if unused)
    uint64_t maxNodes = 0;   // Node limit (0 if unused)
};

struct SearchInfo {
    std::chrono::high_resolution_clock::time_point startTime;
    int allocatedTimeMs = 0;
    uint64_t nodes = 0;
    bool stopped = false;

    void reset(int timeMs = 0) {
        startTime = std::chrono::high_resolution_clock::now();
        allocatedTimeMs = timeMs;
        nodes = 0;
        stopped = false;
    }

    // Check time limit periodically (e.g., every 2048 nodes) to minimize chrono overhead
    inline bool checkLimits() {
        if (stopped) return true;
        if (allocatedTimeMs > 0 && (nodes & 2047) == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            if (elapsed >= allocatedTimeMs) {
                stopped = true;
            }
        }
        return stopped;
    }
};

class Searcher {
public:
    Searcher() = default;

    Move findBestMove(Board& board, const SearchLimits& limits);

private:
    // Core search routines
    int negamax(Board& board, int depth, int alpha, int beta, SearchInfo& info);
    int quiescence(Board& board, int alpha, int beta, SearchInfo& info);

    // Basic move ordering helper (captures first via MVV-LVA)
    void orderMoves(const Board& board, MoveList& moves);
    int scoreMove(const Board& board, Move move);

    Move bestMoveFound;
    Move currentRootMove;
};

#endif // SEARCHER_H