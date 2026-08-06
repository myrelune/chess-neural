#pragma once

#include <cstdint>

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

using Bitboard = uint64_t;

namespace BitboardOps {

    inline void setBit(Bitboard& bb, int square) {
        bb |= (1ULL << square);
    }

    inline void popBit(Bitboard& bb, int square) {
        bb &= ~(1ULL << square);
    }

    inline bool getBit(Bitboard bb, int square) {
        return (bb & (1ULL << square)) != 0;
    }

    inline int countBits(Bitboard bb) {
        #if defined(_MSC_VER)
            return static_cast<int>(__popcnt64(bb));
        #else
            return static_cast<int>(__builtin_popcountll(bb));
        #endif
    }

    inline int getLSB(Bitboard bb) {
        #if defined(_MSC_VER)
            unsigned long index;
            _BitScanForward64(&index, bb);
            return static_cast<int>(index);
        #else
            return __builtin_ctzll(bb);
        #endif
    }

    inline int popLSB(Bitboard& bb) {
        int index = getLSB(bb);
        bb &= bb - 1;
        return index;
    }
}
