// zobrist.h
#pragma once

#include <cstdint>

namespace Zobrist {
    extern uint64_t pieceKeys[12][64];
    extern uint64_t castlingKeys[16];
    extern uint64_t epKeys[8];
    extern uint64_t sideKey;

    void init();
}