#pragma once

#include <cstdint>
#include <array>

using Bitboard = uint64_t;

enum class Color : uint8_t
{
    White,
    Black
};

enum class Piece : uint8_t
{
    None,

    WhitePawn,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,

    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing
};

enum CastlingRights : uint8_t
{
    NoCastling  = 0,

    WhiteKingSide  = 1 << 0,
    WhiteQueenSide = 1 << 1,
    BlackKingSide  = 1 << 2,
    BlackQueenSide = 1 << 3
};

enum class Square : uint8_t
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,

    None = 64
};

constexpr int pieceIndex(Piece piece)
{
    return static_cast<int>(piece) - 1;
}

inline Color pieceColor(Piece piece)
{
    if (piece >= Piece::WhitePawn && piece <= Piece::WhiteKing)
        return Color::White;

    return Color::Black;
}
