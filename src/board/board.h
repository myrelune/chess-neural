#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bitboard.h"
#include "types.h"
#include "../movegen/move.h"

struct Undo {
    Square capturedSquare;
    Piece capturedPiece;
    uint8_t castlingRights;
    Square enPassantSquare;
    int halfmoveClock;
    uint64_t zobristKey;  // saved before the move so unmakeMove can restore it instantly
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

    uint64_t getZobristKey() const { return zobristKey; }

    bool isRepetition() const;
    bool isDraw() const;

    void makeNullMove(Undo& undo);
    void unmakeNullMove(const Undo& undo);

private:
    static constexpr size_t pieceIndex(Piece piece) {
        return static_cast<size_t>(piece) - 1;
    }

    Bitboard pieces[12];
    Piece   mailbox[64]; // O(1) square-to-piece lookup

    Bitboard whitePieces;
    Bitboard blackPieces;
    Bitboard occupied;

    Color sideToMove;
    uint8_t castlingRights;
    Square enPassantSquare;

    int halfmoveClock;
    int fullmoveNumber;
    uint64_t zobristKey;

    std::vector<uint64_t> posHistory;

    uint64_t computeZobristKey() const; // only used during loadFEN / reset

    // Incremental Zobrist helpers
    inline void zxorPiece(Piece p, int sq);
    inline void zxorCastling(uint8_t rights);
    inline void zxorEP(Square sq);
    inline void zxorSide();
};
