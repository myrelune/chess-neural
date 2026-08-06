#pragma once

#include <cstdint>
#include <string>

#include "bitboard.h"
#include "types.h"
#include "../movegen/move.h"

struct Undo {
    Square capturedSquare;
    Piece capturedPiece;
    uint8_t castlingRights;
    Square enPassantSquare;
    int halfmoveClock;
};

class Board {
public:
    Board();

    void clear();
    void reset();

    Piece pieceAt(Square square) const;
    void setPiece(Square square, Piece piece);
    void removePiece(Square square);

    void printBoard() const;
    void loadFEN(const std::string& fen);

    Color getSideToMove() const { return sideToMove; }
    Bitboard getOccupied() const { return occupied; }
    Bitboard getWhitePieces() const { return whitePieces; }
    Bitboard getBlackPieces() const { return blackPieces; }

    Bitboard getPieces(Color color) const {
        return (color == Color::White) ? whitePieces : blackPieces;
    }

    Bitboard getPieces(Piece piece) const {
        return pieces[pieceIndex(piece)];
    }

    uint8_t getCastlingRights() const { return castlingRights; }
    Square getEnPassantSquare() const { return enPassantSquare; }

    bool makeMove(const Move& move, Undo& undo);
    void unmakeMove(const Move& move, const Undo& undo);

    bool isSquareAttacked(Square square, Color attackerColor) const;

private:
    static constexpr size_t pieceIndex(Piece piece) {
        return static_cast<size_t>(piece) - 1;
    }

    Bitboard pieces[12];

    Bitboard whitePieces;
    Bitboard blackPieces;
    Bitboard occupied;

    Color sideToMove;
    uint8_t castlingRights;
    Square enPassantSquare;

    int halfmoveClock;
    int fullmoveNumber;
};
