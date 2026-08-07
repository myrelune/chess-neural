#ifndef SEARCHER_H
#define SEARCHER_H

#include "../board/board.h"
#include "../movegen/movegen.h"
#include "../eval/evaluate.h"

#include <chrono>
#include <cstdint>
#include <vector>

constexpr int MATE_SCORE = 40000;
constexpr int INFINITY_SCORE = 50000;

// --- TRANSPOSITION TABLE DEFINITIONS ---
enum class Bound : uint8_t {
    None = 0,
    Exact = 1,
    Lower = 2,
    Upper = 3
};

struct TTEntry {
    uint64_t key;
    int score;
    int depth;
    Bound bound;
    Move bestMove;
};

class TranspositionTable {
private:
    std::vector<TTEntry> table;
    size_t numEntries;

public:
    TranspositionTable(size_t megabytes = 16) {
        resize(megabytes);
    }

    void resize(size_t megabytes) {
        size_t bytes = megabytes * 1024 * 1024;
        numEntries = bytes / sizeof(TTEntry);
        table.clear();
        table.resize(numEntries, {0, 0, 0, Bound::None, Move()});
    }

    void clear() {
        std::fill(table.begin(), table.end(), TTEntry{0, 0, 0, Bound::None, Move()});
    }

    void store(uint64_t key, int score, int depth, Bound bound, Move bestMove) {
        size_t index = key % numEntries;
        const TTEntry& existing = table[index];

        Move moveToStore = (bestMove != Move()) ? bestMove : (existing.key == key ? existing.bestMove : Move());

        if (existing.key != key || depth >= existing.depth) {
            table[index] = {key, score, depth, bound, moveToStore};
        }
    }

    Move getStoredMove(uint64_t key) const {
        size_t index = key % numEntries;
        const TTEntry& entry = table[index];
        if (entry.key == key) {
            return entry.bestMove;
        }
        return Move();
    }

    bool probe(uint64_t key, int depth, int alpha, int beta, int& outScore, Move& outBestMove) {
        size_t index = key % numEntries;
        const TTEntry& entry = table[index];

        if (entry.key == key) {
            outBestMove = entry.bestMove;

            if (entry.depth >= depth) {
                if (entry.bound == Bound::Exact) {
                    outScore = entry.score;
                    return true;
                }
                if (entry.bound == Bound::Lower && entry.score >= beta) {
                    outScore = entry.score;
                    return true; // Beta cutoff
                }
                if (entry.bound == Bound::Upper && entry.score <= alpha) {
                    outScore = entry.score;
                    return true; // Alpha cutoff
                }
            }
        }
        return false;
    }
};

struct SearchLimits {
    int maxDepth = 64;
    int moveTimeMs = 0;
    uint64_t maxNodes = 0;
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
    Searcher() : tt(16) {} // Initialize with 16MB Transposition Table by default

    Move findBestMove(Board& board, const SearchLimits& limits);
    void newGame() { tt.clear(); bestMoveFound = Move(); }

private:
    // Core search routines
    int negamax(Board& board, int depth, int alpha, int beta, SearchInfo& info, int ply);
    int quiescence(Board& board, int alpha, int beta, SearchInfo& info);

    // Advanced move ordering helper
    void orderMoves(const Board& board, MoveList& moves, Move ttMove, int ply);
    int scoreMove(const Board& board, Move move, Move ttMove, int ply);

    void clearHeuristics();

    TranspositionTable tt;
    Move bestMoveFound;
    Move currentRootMove;

    Move killerMoves[64][2];
    int historyTable[2][64][64];
};

#endif // SEARCHER_H