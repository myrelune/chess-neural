#include "board.h"
#include "bitboard.h"

#include <iostream>
#include <sstream>
#include <cctype>

Board::Board() {
    reset();
}

void Board::clear() {
    for (int i = 0; i < 12; i++)
        pieces[i] = 0;

    whitePieces = 0;
    blackPieces = 0;
    occupied = 0;

    sideToMove = Color::White;
    castlingRights = 0;
    enPassantSquare = Square::None;

    halfmoveClock = 0;
    fullmoveNumber = 1;
}

void Board::reset() {
    clear();

    sideToMove = Color::White;
    enPassantSquare = Square::None;

    // White pieces
    setPiece(Square::A1, Piece::WhiteRook);
    setPiece(Square::B1, Piece::WhiteKnight);
    setPiece(Square::C1, Piece::WhiteBishop);
    setPiece(Square::D1, Piece::WhiteQueen);
    setPiece(Square::E1, Piece::WhiteKing);
    setPiece(Square::F1, Piece::WhiteBishop);
    setPiece(Square::G1, Piece::WhiteKnight);
    setPiece(Square::H1, Piece::WhiteRook);

    setPiece(Square::A2, Piece::WhitePawn);
    setPiece(Square::B2, Piece::WhitePawn);
    setPiece(Square::C2, Piece::WhitePawn);
    setPiece(Square::D2, Piece::WhitePawn);
    setPiece(Square::E2, Piece::WhitePawn);
    setPiece(Square::F2, Piece::WhitePawn);
    setPiece(Square::G2, Piece::WhitePawn);
    setPiece(Square::H2, Piece::WhitePawn);

    // Black pieces
    setPiece(Square::A8, Piece::BlackRook);
    setPiece(Square::B8, Piece::BlackKnight);
    setPiece(Square::C8, Piece::BlackBishop);
    setPiece(Square::D8, Piece::BlackQueen);
    setPiece(Square::E8, Piece::BlackKing);
    setPiece(Square::F8, Piece::BlackBishop);
    setPiece(Square::G8, Piece::BlackKnight);
    setPiece(Square::H8, Piece::BlackRook);

    setPiece(Square::A7, Piece::BlackPawn);
    setPiece(Square::B7, Piece::BlackPawn);
    setPiece(Square::C7, Piece::BlackPawn);
    setPiece(Square::D7, Piece::BlackPawn);
    setPiece(Square::E7, Piece::BlackPawn);
    setPiece(Square::F7, Piece::BlackPawn);
    setPiece(Square::G7, Piece::BlackPawn);
    setPiece(Square::H7, Piece::BlackPawn);

    castlingRights =
        WhiteKingSide |
        WhiteQueenSide |
        BlackKingSide |
        BlackQueenSide;
}

void Board::setPiece(Square square, Piece piece) {
    if (piece == Piece::None)
        return;

    int sq = static_cast<int>(square);

    BitboardOps::setBit(pieces[pieceIndex(piece)], sq);

    if (pieceColor(piece) == Color::White)
        BitboardOps::setBit(whitePieces, sq);
    else
        BitboardOps::setBit(blackPieces, sq);

    BitboardOps::setBit(occupied, sq);
}

Piece Board::pieceAt(Square square) const {
    int sq = static_cast<int>(square);

    if (sq < 0 || sq >= 64)
        return Piece::None;

    for (int i = 0; i < 12; i++) {
        if (BitboardOps::getBit(pieces[i], sq))
            return static_cast<Piece>(i + 1);
    }

    return Piece::None;
}

void Board::removePiece(Square square) {
    Piece piece = pieceAt(square);

    if (piece == Piece::None)
        return;

    int sq = static_cast<int>(square);

    BitboardOps::popBit(pieces[pieceIndex(piece)], sq);

    if (pieceColor(piece) == Color::White)
        BitboardOps::popBit(whitePieces, sq);
    else
        BitboardOps::popBit(blackPieces, sq);

    BitboardOps::popBit(occupied, sq);
}

void Board::printBoard() const {
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << " ";

        for (int file = 0; file < 8; file++) {
            Square square = static_cast<Square>(rank * 8 + file);
            Piece piece = pieceAt(square);
            char symbol = '.';

            switch (piece) {
                case Piece::WhitePawn:   symbol = 'P'; break;
                case Piece::WhiteKnight: symbol = 'N'; break;
                case Piece::WhiteBishop: symbol = 'B'; break;
                case Piece::WhiteRook:   symbol = 'R'; break;
                case Piece::WhiteQueen:  symbol = 'Q'; break;
                case Piece::WhiteKing:   symbol = 'K'; break;

                case Piece::BlackPawn:   symbol = 'p'; break;
                case Piece::BlackKnight: symbol = 'n'; break;
                case Piece::BlackBishop: symbol = 'b'; break;
                case Piece::BlackRook:   symbol = 'r'; break;
                case Piece::BlackQueen:  symbol = 'q'; break;
                case Piece::BlackKing:   symbol = 'k'; break;

                default: break;
            }

            std::cout << symbol << " ";
        }

        std::cout << "\n";
    }

    std::cout << "\n a b c d e f g h\n";
}

void Board::loadFEN(const std::string& fen) {
    clear();

    std::istringstream iss(fen);
    std::string piecePlacement, activeColor, castling, enPassant;
    std::string halfmove = "0", fullmove = "1";

    if (!(iss >> piecePlacement >> activeColor >> castling >> enPassant)) {
        return;
    }
    iss >> halfmove;
    iss >> fullmove;

    int rank = 7;
    int file = 0;

    for (char c : piecePlacement) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            file += (c - '0');
        } else {
            Square square = static_cast<Square>(rank * 8 + file);
            Piece piece = Piece::None;

            switch (c) {
                case 'P': piece = Piece::WhitePawn; break;
                case 'N': piece = Piece::WhiteKnight; break;
                case 'B': piece = Piece::WhiteBishop; break;
                case 'R': piece = Piece::WhiteRook; break;
                case 'Q': piece = Piece::WhiteQueen; break;
                case 'K': piece = Piece::WhiteKing; break;

                case 'p': piece = Piece::BlackPawn; break;
                case 'n': piece = Piece::BlackKnight; break;
                case 'b': piece = Piece::BlackBishop; break;
                case 'r': piece = Piece::BlackRook; break;
                case 'q': piece = Piece::BlackQueen; break;
                case 'k': piece = Piece::BlackKing; break;
                default: break;
            }

            if (piece != Piece::None) {
                setPiece(square, piece);
            }
            file++;
        }
    }

    sideToMove = (activeColor == "w") ? Color::White : Color::Black;

    castlingRights = 0;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castlingRights |= WhiteKingSide; break;
                case 'Q': castlingRights |= WhiteQueenSide; break;
                case 'k': castlingRights |= BlackKingSide; break;
                case 'q': castlingRights |= BlackQueenSide; break;
                default: break;
            }
        }
    }

    if (enPassant == "-") {
        enPassantSquare = Square::None;
    } else if (enPassant.length() >= 2) {
        int epFile = enPassant[0] - 'a';
        int epRank = enPassant[1] - '1';
        enPassantSquare = static_cast<Square>(epRank * 8 + epFile);
    }

    try { halfmoveClock = std::stoi(halfmove); } catch (...) { halfmoveClock = 0; }
    try { fullmoveNumber = std::stoi(fullmove); } catch (...) { fullmoveNumber = 1; }
}

bool Board::makeMove(const Move& move, Undo& undo) {
    undo.castlingRights = castlingRights;
    undo.enPassantSquare = enPassantSquare;
    undo.halfmoveClock = halfmoveClock;
    undo.capturedPiece = Piece::None;
    undo.capturedSquare = move.to;

    Piece movingPiece = pieceAt(move.from);
    if (movingPiece == Piece::None) return false;

    Piece targetPiece = pieceAt(move.to);

    // Handle captures
    if (targetPiece != Piece::None) {
        undo.capturedPiece = targetPiece;
        removePiece(move.to);
    }
    // Handle En Passant capture
    else if ((movingPiece == Piece::WhitePawn || movingPiece == Piece::BlackPawn) && move.to == enPassantSquare) {
        Square epPawnSquare = (sideToMove == Color::White) ?
            static_cast<Square>(static_cast<int>(move.to) - 8) :
            static_cast<Square>(static_cast<int>(move.to) + 8);
        undo.capturedSquare = epPawnSquare;
        undo.capturedPiece = pieceAt(epPawnSquare);
        removePiece(epPawnSquare);
    }

    // Move primary piece
    removePiece(move.from);
    setPiece(move.to, move.promotion != Piece::None ? move.promotion : movingPiece);

    // Handle Castling Rook move
    if (movingPiece == Piece::WhiteKing) {
        if (move.from == Square::E1 && move.to == Square::G1) {
            removePiece(Square::H1);
            setPiece(Square::F1, Piece::WhiteRook);
        } else if (move.from == Square::E1 && move.to == Square::C1) {
            removePiece(Square::A1);
            setPiece(Square::D1, Piece::WhiteRook);
        }
    } else if (movingPiece == Piece::BlackKing) {
        if (move.from == Square::E8 && move.to == Square::G8) {
            removePiece(Square::H8);
            setPiece(Square::F8, Piece::BlackRook);
        } else if (move.from == Square::E8 && move.to == Square::C8) {
            removePiece(Square::A8);
            setPiece(Square::D8, Piece::BlackRook);
        }
    }

    // Reset or update En Passant square
    enPassantSquare = Square::None;
    if (movingPiece == Piece::WhitePawn && static_cast<int>(move.to) - static_cast<int>(move.from) == 16) {
        enPassantSquare = static_cast<Square>(static_cast<int>(move.from) + 8);
    } else if (movingPiece == Piece::BlackPawn && static_cast<int>(move.from) - static_cast<int>(move.to) == 16) {
        enPassantSquare = static_cast<Square>(static_cast<int>(move.from) - 8);
    }

    if (movingPiece == Piece::WhiteKing) {
        castlingRights &= ~(WhiteKingSide | WhiteQueenSide);
    } else if (movingPiece == Piece::BlackKing) {
        castlingRights &= ~(BlackKingSide | BlackQueenSide);
    }

    if (move.from == Square::A1 || move.to == Square::A1) castlingRights &= ~WhiteQueenSide;
    if (move.from == Square::H1 || move.to == Square::H1) castlingRights &= ~WhiteKingSide;
    if (move.from == Square::A8 || move.to == Square::A8) castlingRights &= ~BlackQueenSide;
    if (move.from == Square::H8 || move.to == Square::H8) castlingRights &= ~BlackKingSide;

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    return true;
}

void Board::unmakeMove(const Move& move, const Undo& undo) {
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;

    Piece movedPiece = pieceAt(move.to);

    if (move.promotion != Piece::None) {
        removePiece(move.to);
        Piece originalPawn = (sideToMove == Color::White) ? Piece::WhitePawn : Piece::BlackPawn;
        setPiece(move.from, originalPawn);
    } else {
        removePiece(move.to);
        setPiece(move.from, movedPiece);
    }

    // Restore Rook if castling
    if (movedPiece == Piece::WhiteKing) {
        if (move.from == Square::E1 && move.to == Square::G1) {
            removePiece(Square::F1);
            setPiece(Square::H1, Piece::WhiteRook);
        } else if (move.from == Square::E1 && move.to == Square::C1) {
            removePiece(Square::D1);
            setPiece(Square::A1, Piece::WhiteRook);
        }
    } else if (movedPiece == Piece::BlackKing) {
        if (move.from == Square::E8 && move.to == Square::G8) {
            removePiece(Square::F8);
            setPiece(Square::H8, Piece::BlackRook);
        } else if (move.from == Square::E8 && move.to == Square::C8) {
            removePiece(Square::D8);
            setPiece(Square::A8, Piece::BlackRook);
        }
    }

    if (undo.capturedPiece != Piece::None) {
        setPiece(undo.capturedSquare, undo.capturedPiece);
    }

    castlingRights = undo.castlingRights;
    enPassantSquare = undo.enPassantSquare;
    halfmoveClock = undo.halfmoveClock;
}

bool Board::isSquareAttacked(Square square, Color attackerColor) const {
    int targetIdx = static_cast<int>(square);
    int targetRank = targetIdx / 8;
    int targetFile = targetIdx % 8;

    // Pawn attacks
    if (attackerColor == Color::White) {
        Piece attackerPawn = Piece::WhitePawn;
        Bitboard pawns = getPieces(attackerPawn);
        int pawnR = targetRank - 1;
        if (pawnR >= 0) {
            if (targetFile > 0 && BitboardOps::getBit(pawns, pawnR * 8 + (targetFile - 1))) return true;
            if (targetFile < 7 && BitboardOps::getBit(pawns, pawnR * 8 + (targetFile + 1))) return true;
        }
    } else {
        Piece attackerPawn = Piece::BlackPawn;
        Bitboard pawns = getPieces(attackerPawn);
        int pawnR = targetRank + 1;
        if (pawnR < 8) {
            if (targetFile > 0 && BitboardOps::getBit(pawns, pawnR * 8 + (targetFile - 1))) return true;
            if (targetFile < 7 && BitboardOps::getBit(pawns, pawnR * 8 + (targetFile + 1))) return true;
        }
    }

    // Knight attacks
    Piece attackerKnight = (attackerColor == Color::White) ? Piece::WhiteKnight : Piece::BlackKnight;
    Bitboard knights = getPieces(attackerKnight);
    constexpr int knightOffsets[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    for (auto& offset : knightOffsets) {
        int r = targetRank + offset[0];
        int f = targetFile + offset[1];
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            if (BitboardOps::getBit(knights, r * 8 + f)) return true;
        }
    }

    // King attacks
    Piece attackerKing = (attackerColor == Color::White) ? Piece::WhiteKing : Piece::BlackKing;
    Bitboard kings = getPieces(attackerKing);
    constexpr int kingOffsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& offset : kingOffsets) {
        int r = targetRank + offset[0];
        int f = targetFile + offset[1];
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            if (BitboardOps::getBit(kings, r * 8 + f)) return true;
        }
    }

    // Straight Sliders (Rook / Queen)
    Piece attackerRook = (attackerColor == Color::White) ? Piece::WhiteRook : Piece::BlackRook;
    Piece attackerQueen = (attackerColor == Color::White) ? Piece::WhiteQueen : Piece::BlackQueen;
    Bitboard straightSliders = getPieces(attackerRook) | getPieces(attackerQueen);
    constexpr int rookOffsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& offset : rookOffsets) {
        int r = targetRank;
        int f = targetFile;
        while (true) {
            r += offset[0];
            f += offset[1];
            if (r < 0 || r > 7 || f < 0 || f > 7) break;
            int sq = r * 8 + f;
            if (BitboardOps::getBit(occupied, sq)) {
                if (BitboardOps::getBit(straightSliders, sq)) return true;
                break;
            }
        }
    }

    // Diagonal Sliders (Bishop / Queen)
    Piece attackerBishop = (attackerColor == Color::White) ? Piece::WhiteBishop : Piece::BlackBishop;
    Bitboard diagSliders = getPieces(attackerBishop) | getPieces(attackerQueen);
    constexpr int bishopOffsets[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& offset : bishopOffsets) {
        int r = targetRank;
        int f = targetFile;
        while (true) {
            r += offset[0];
            f += offset[1];
            if (r < 0 || r > 7 || f < 0 || f > 7) break;
            int sq = r * 8 + f;
            if (BitboardOps::getBit(occupied, sq)) {
                if (BitboardOps::getBit(diagSliders, sq)) return true;
                break;
            }
        }
    }

    return false;
}
