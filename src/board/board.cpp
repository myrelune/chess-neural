#include "board.h"
#include "bitboard.h"
#include "../zobrist/zobrist.h"
#include "../attacks/attacks.h"
#include <vector>

#include <iostream>
#include <sstream>
#include <cctype>

// ---------------------------------------------------------------------------
// Incremental Zobrist helpers — tiny private methods so callsites stay clean
// ---------------------------------------------------------------------------
inline void Board::zxorPiece(Piece p, int sq) {
    zobristKey ^= Zobrist::pieceKeys[pieceIndex(p)][sq];
}
inline void Board::zxorCastling(uint8_t rights) {
    zobristKey ^= Zobrist::castlingKeys[rights];
}
inline void Board::zxorEP(Square sq) {
    if (sq != Square::None)
        zobristKey ^= Zobrist::epKeys[static_cast<int>(sq) % 8];
}
inline void Board::zxorSide() {
    zobristKey ^= Zobrist::sideKey;
}

// ---------------------------------------------------------------------------
Board::Board() {
    reset();
}

void Board::clear() {
    for (int i = 0; i < 12; i++)
        pieces[i] = 0;

    for (int i = 0; i < 64; i++)
        mailbox[i] = Piece::None;

    whitePieces = 0;
    blackPieces = 0;
    occupied = 0;

    sideToMove = Color::White;
    castlingRights = 0;
    enPassantSquare = Square::None;

    halfmoveClock = 0;
    fullmoveNumber = 1;
    zobristKey = 0;
    posHistory.clear();
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

    zobristKey = computeZobristKey();
}

// ---------------------------------------------------------------------------
// Low-level piece placement — keeps bitboards + mailbox in sync.
// NOTE: does NOT touch zobristKey; callers handle Zobrist incrementally.
// ---------------------------------------------------------------------------
void Board::setPiece(Square square, Piece piece) {
    if (piece == Piece::None)
        return;

    int sq = static_cast<int>(square);

    BitboardOps::setBit(pieces[pieceIndex(piece)], sq);
    mailbox[sq] = piece;

    if (pieceColor(piece) == Color::White)
        BitboardOps::setBit(whitePieces, sq);
    else
        BitboardOps::setBit(blackPieces, sq);

    BitboardOps::setBit(occupied, sq);
}

// O(1) — just read the mailbox
Piece Board::pieceAt(Square square) const {
    int sq = static_cast<int>(square);
    if (sq < 0 || sq >= 64)
        return Piece::None;
    return mailbox[sq];
}

void Board::removePiece(Square square) {
    int sq = static_cast<int>(square);
    Piece piece = mailbox[sq];

    if (piece == Piece::None)
        return;

    BitboardOps::popBit(pieces[pieceIndex(piece)], sq);
    mailbox[sq] = Piece::None;

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

    // Full recompute once after loading — all subsequent updates are incremental
    zobristKey = computeZobristKey();
}

// ---------------------------------------------------------------------------
// makeMove — fully incremental Zobrist updates, O(1) pieceAt via mailbox
// ---------------------------------------------------------------------------
bool Board::makeMove(const Move& move, Undo& undo) {
    // Save full state needed for unmake
    undo.castlingRights  = castlingRights;
    undo.enPassantSquare = enPassantSquare;
    undo.halfmoveClock   = halfmoveClock;
    undo.capturedPiece   = Piece::None;
    undo.capturedSquare  = move.to;
    undo.zobristKey      = zobristKey; // single save — unmakeMove just restores this

    Piece movingPiece = mailbox[static_cast<int>(move.from)];
    if (movingPiece == Piece::None) return false;

    Piece targetPiece = mailbox[static_cast<int>(move.to)];

    // --- Incremental Zobrist: strip out everything that's about to change ---

    // Side key flips every move
    zxorSide();

    // Old castling rights
    zxorCastling(castlingRights);

    // Old en passant file (if any)
    zxorEP(enPassantSquare);

    // Moving piece leaves its source square
    zxorPiece(movingPiece, static_cast<int>(move.from));

    // Handle captures
    if (targetPiece != Piece::None) {
        undo.capturedPiece = targetPiece;
        zxorPiece(targetPiece, static_cast<int>(move.to));
        removePiece(move.to);
    }
    // Handle en passant capture
    else if ((movingPiece == Piece::WhitePawn || movingPiece == Piece::BlackPawn)
             && move.to == enPassantSquare) {
        Square epPawnSquare = (sideToMove == Color::White)
            ? static_cast<Square>(static_cast<int>(move.to) - 8)
            : static_cast<Square>(static_cast<int>(move.to) + 8);
        undo.capturedSquare = epPawnSquare;
        undo.capturedPiece  = mailbox[static_cast<int>(epPawnSquare)];
        zxorPiece(undo.capturedPiece, static_cast<int>(epPawnSquare));
        removePiece(epPawnSquare);
    }

    // Move primary piece
    removePiece(move.from);
    Piece placedPiece = (move.promotion != Piece::None) ? move.promotion : movingPiece;
    setPiece(move.to, placedPiece);
    zxorPiece(placedPiece, static_cast<int>(move.to));

    // Handle castling rook
    if (movingPiece == Piece::WhiteKing) {
        if (move.from == Square::E1 && move.to == Square::G1) {
            zxorPiece(Piece::WhiteRook, static_cast<int>(Square::H1));
            removePiece(Square::H1);
            setPiece(Square::F1, Piece::WhiteRook);
            zxorPiece(Piece::WhiteRook, static_cast<int>(Square::F1));
        } else if (move.from == Square::E1 && move.to == Square::C1) {
            zxorPiece(Piece::WhiteRook, static_cast<int>(Square::A1));
            removePiece(Square::A1);
            setPiece(Square::D1, Piece::WhiteRook);
            zxorPiece(Piece::WhiteRook, static_cast<int>(Square::D1));
        }
    } else if (movingPiece == Piece::BlackKing) {
        if (move.from == Square::E8 && move.to == Square::G8) {
            zxorPiece(Piece::BlackRook, static_cast<int>(Square::H8));
            removePiece(Square::H8);
            setPiece(Square::F8, Piece::BlackRook);
            zxorPiece(Piece::BlackRook, static_cast<int>(Square::F8));
        } else if (move.from == Square::E8 && move.to == Square::C8) {
            zxorPiece(Piece::BlackRook, static_cast<int>(Square::A8));
            removePiece(Square::A8);
            setPiece(Square::D8, Piece::BlackRook);
            zxorPiece(Piece::BlackRook, static_cast<int>(Square::D8));
        }
    }

    // Update en passant square
    enPassantSquare = Square::None;
    if (movingPiece == Piece::WhitePawn
        && static_cast<int>(move.to) - static_cast<int>(move.from) == 16) {
        enPassantSquare = static_cast<Square>(static_cast<int>(move.from) + 8);
    } else if (movingPiece == Piece::BlackPawn
               && static_cast<int>(move.from) - static_cast<int>(move.to) == 16) {
        enPassantSquare = static_cast<Square>(static_cast<int>(move.from) - 8);
    }

    // Update castling rights
    if (movingPiece == Piece::WhiteKing)
        castlingRights &= ~(WhiteKingSide | WhiteQueenSide);
    else if (movingPiece == Piece::BlackKing)
        castlingRights &= ~(BlackKingSide | BlackQueenSide);

    if (move.from == Square::A1 || move.to == Square::A1) castlingRights &= ~WhiteQueenSide;
    if (move.from == Square::H1 || move.to == Square::H1) castlingRights &= ~WhiteKingSide;
    if (move.from == Square::A8 || move.to == Square::A8) castlingRights &= ~BlackQueenSide;
    if (move.from == Square::H8 || move.to == Square::H8) castlingRights &= ~BlackKingSide;

    // XOR in the new castling rights and new EP
    zxorCastling(castlingRights);
    zxorEP(enPassantSquare);

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    posHistory.push_back(zobristKey);
    return true;
}

// ---------------------------------------------------------------------------
// unmakeMove — restore Zobrist key from Undo in one assignment (zero recompute)
// ---------------------------------------------------------------------------
void Board::unmakeMove(const Move& move, const Undo& undo) {
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;

    Piece movedPiece = mailbox[static_cast<int>(move.to)];

    if (move.promotion != Piece::None) {
        removePiece(move.to);
        Piece originalPawn = (sideToMove == Color::White) ? Piece::WhitePawn : Piece::BlackPawn;
        setPiece(move.from, originalPawn);
    } else {
        removePiece(move.to);
        setPiece(move.from, movedPiece);
    }

    // Restore rook if castling
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

    castlingRights  = undo.castlingRights;
    enPassantSquare = undo.enPassantSquare;
    halfmoveClock   = undo.halfmoveClock;
    zobristKey      = undo.zobristKey; // O(1) restore — no recompute needed
    if (!posHistory.empty()) posHistory.pop_back();
}

// ---------------------------------------------------------------------------
bool Board::isRepetition() const {
    // Current position key = zobristKey.
    // Check if it has appeared at least once before in history (twofold = draw)
    int count = 0;
    for (int i = (int)posHistory.size() - 2; i >= 0; i -= 2) {
        if (posHistory[i] == zobristKey) {
            count++;
            if (count >= 1) return true; // twofold repetition
        }
        // Stop looking back past irreversible moves (pawn move / capture resets halfmove clock)
        // we approximate this by limiting to halfmoveClock plies back
        if ((int)posHistory.size() - i > halfmoveClock + 1) break;
    }
    return false;
}

bool Board::isDraw() const {
    // 50-move rule
    if (halfmoveClock >= 100) return true;
    // Insufficient material: only kings remain
    Bitboard allPieces = occupied;
    if (allPieces == (getPieces(Piece::WhiteKing) | getPieces(Piece::BlackKing))) return true;
    return false;
}

bool Board::isSquareAttacked(Square square, Color attackerColor) const {
    return Attacks::isSquareAttacked(*this, square, attackerColor);
}

// ---------------------------------------------------------------------------
// Full Zobrist recompute — only called from reset() and loadFEN()
// ---------------------------------------------------------------------------
uint64_t Board::computeZobristKey() const {
    uint64_t key = 0;

    for (int sq = 0; sq < 64; ++sq) {
        Piece p = mailbox[sq];
        if (p != Piece::None)
            key ^= Zobrist::pieceKeys[pieceIndex(p)][sq];
    }

    key ^= Zobrist::castlingKeys[castlingRights];

    if (enPassantSquare != Square::None)
        key ^= Zobrist::epKeys[static_cast<int>(enPassantSquare) % 8];

    if (sideToMove == Color::Black)
        key ^= Zobrist::sideKey;

    return key;
}

// ---------------------------------------------------------------------------
void Board::makeNullMove(Undo& undo) {
    undo.castlingRights  = castlingRights;
    undo.enPassantSquare = enPassantSquare;
    undo.halfmoveClock   = halfmoveClock;
    undo.capturedPiece   = Piece::None;
    undo.capturedSquare  = Square::None;
    undo.zobristKey      = zobristKey;

    // Incremental: remove EP, flip side
    zxorEP(enPassantSquare);
    enPassantSquare = Square::None;
    zxorSide();

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
}

void Board::unmakeNullMove(const Undo& undo) {
    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;

    castlingRights  = undo.castlingRights;
    enPassantSquare = undo.enPassantSquare;
    halfmoveClock   = undo.halfmoveClock;
    zobristKey      = undo.zobristKey;
}