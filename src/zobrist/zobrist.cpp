// zobrist.cpp
#include "zobrist.h"

namespace Zobrist {

    uint64_t pieceKeys[12][64];
    uint64_t castlingKeys[16];
    uint64_t epKeys[8];
    uint64_t sideKey;

    // Simple 64-bit Xorshift PRNG to generate pseudo-random numbers locally
    class PRNG {
    private:
        uint64_t state;
    public:
        PRNG(uint64_t seed) : state(seed) {}

        uint64_t next64() {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            return state * 0x2545F4914F6CDD1DULL;
        }
    };

    void init() {
        // Use a fixed seed so keys are identical across program runs
        PRNG prng(1070372ull);

        // 1. Initialize piece keys (12 pieces, 64 squares)
        for (int piece = 0; piece < 12; ++piece) {
            for (int sq = 0; sq < 64; ++sq) {
                pieceKeys[piece][sq] = prng.next64();
            }
        }

        // 2. Initialize castling rights keys (16 possible bitmask combinations)
        for (int i = 0; i < 16; ++i) {
            castlingKeys[i] = prng.next64();
        }

        // 3. Initialize en passant file keys (Files A through H -> 8 files)
        for (int file = 0; file < 8; ++file) {
            epKeys[file] = prng.next64();
        }

        // 4. Initialize side to move key
        sideKey = prng.next64();
    }

}