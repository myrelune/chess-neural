#include "polyglot.h"
#include <vector>
#include <algorithm>
#include <cstring>

#if __has_include("embedded_book.h")
#include "embedded_book.h"
#else
namespace EmbeddedBook {
    constexpr unsigned char data[] = {0x00};
}
#endif

#if defined(_MSC_VER)
#define bswap_64(x) _byteswap_uint64(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_16(x) _byteswap_ushort(x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define bswap_64(x) OSSwapBigToHostInt64(x)
#define bswap_32(x) ntohl(x)
#define bswap_16(x) OSSwapBigToHostInt16(x)
#else
#include <byteswap.h>
#define bswap_64(x) bswap_64(x)
#define bswap_32(x) ntohl(x)
#define bswap_16(x) bswap_16(x)
#endif

inline uint32_t netToHost32(uint32_t net) {
    return bswap_32(net);
}

bool OpeningBook::load(const std::string& filepath) {
    return false;
}

// Load directly from the compiled-in byte array
bool OpeningBook::loadEmbedded() {
    size_t size = sizeof(EmbeddedBook::data);
    if (size == 0) return false;

    size_t numEntries = size / sizeof(PolyEntry);
    entries.resize(numEntries);

    std::memcpy(entries.data(), EmbeddedBook::data, size);

    for (auto& entry : entries) {
        entry.key    = bswap_64(entry.key);
        entry.move   = bswap_16(entry.move);
        entry.weight = bswap_16(entry.weight);
        entry.learn  = netToHost32(entry.learn);
    }
    return true;
}

std::vector<PolyEntry> OpeningBook::getBookMoves(uint64_t boardKey) const {
    std::vector<PolyEntry> matchingMoves;
    if (entries.empty()) return matchingMoves;

    PolyEntry target{boardKey, 0, 0, 0};
    auto it = std::lower_bound(entries.begin(), entries.end(), target,
        [](const PolyEntry& a, const PolyEntry& b) {
            return a.key < b.key;
        });

    while (it != entries.end() && it->key == boardKey) {
        matchingMoves.push_back(*it);
        ++it;
    }

    return matchingMoves;
}

Move OpeningBook::decodePolyglotMove(uint16_t rawMove, const Board& board) const {
    int toFile   = (rawMove >> 0) & 7;
    int toRank   = (rawMove >> 3) & 7;
    int fromFile = (rawMove >> 6) & 7;
    int fromRank = (rawMove >> 9) & 7;
    int promo    = (rawMove >> 12) & 7;

    int fromSq = fromRank * 8 + fromFile;
    int toSq   = toRank * 8 + toFile;

    Piece promoPiece = Piece::None;
    if (promo > 0) {
        bool isWhite = (board.getSideToMove() == Color::White);
        switch (promo) {
            case 1: promoPiece = isWhite ? Piece::WhiteKnight : Piece::BlackKnight; break;
            case 2: promoPiece = isWhite ? Piece::WhiteBishop : Piece::BlackBishop; break;
            case 3: promoPiece = isWhite ? Piece::WhiteRook   : Piece::BlackRook;   break;
            case 4: promoPiece = isWhite ? Piece::WhiteQueen  : Piece::BlackQueen;  break;
        }
    }

    return Move{
        static_cast<Square>(fromSq),
        static_cast<Square>(toSq),
        promoPiece
    };
}
