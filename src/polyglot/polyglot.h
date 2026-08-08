#ifndef POLYGLOT_H
#define POLYGLOT_H

#include <cstdint>
#include <vector>
#include <string>
#include "../board/board.h"

struct PolyEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};

class OpeningBook {
private:
    std::vector<PolyEntry> entries;
public:
    bool load(const std::string& filepath);
    bool loadEmbedded();
    std::vector<PolyEntry> getBookMoves(uint64_t boardKey) const;
    Move decodePolyglotMove(uint16_t rawMove, const Board& board) const;
};

#endif
