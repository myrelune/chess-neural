#pragma once

#include "../board/types.h"
#include <array>
#include <string>

struct Move {
    Square from = Square::None;
    Square to = Square::None;
    Piece promotion = Piece::None;

    // Getter helpers for Searcher
    Square getFromSquare() const { return from; }
    Square getToSquare() const { return to; }

    // Equality check for move ordering and best move tracking
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotion == other.promotion;
    }

    bool operator!=(const Move& other) const {
        return !(*this == other);
    }
};

struct MoveList {
    std::array<Move, 256> moves;
    int count = 0;

    void add(Square from, Square to, Piece promotion = Piece::None) {
        moves[count++] = {from, to, promotion};
    }

    auto begin() const { return moves.begin(); }
    auto end() const { return moves.begin() + count; }
};

inline std::string moveToUci(const Move& move) {
    std::string s = "";

    int fromIdx = static_cast<int>(move.from);
    s += ('a' + (fromIdx % 8));
    s += ('1' + (fromIdx / 8));

    int toIdx = static_cast<int>(move.to);
    s += ('a' + (toIdx % 8));
    s += ('1' + (toIdx / 8));

    if (move.promotion != Piece::None) {
        switch (move.promotion) {
            case Piece::WhiteQueen:  case Piece::BlackQueen:  s += 'q'; break;
            case Piece::WhiteRook:   case Piece::BlackRook:   s += 'r'; break;
            case Piece::WhiteBishop: case Piece::BlackBishop: s += 'b'; break;
            case Piece::WhiteKnight: case Piece::BlackKnight: s += 'n'; break;
            default: break;
        }
    }

    return s;
}