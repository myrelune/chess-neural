#include "movegen.h"
#include "../board/bitboard.h"
#include "../attacks/attacks.h"

namespace MoveGen {

    // Fast Slider Generation via Magic Bitboards
    void generateSliderMoves(const Board& board, MoveList& moveList, Color side, Piece pieceType) {
        Bitboard pieces = board.getPieces(pieceType);
        Bitboard friendlyPieces = (side == Color::White) ? board.getWhitePieces() : board.getBlackPieces();
        Bitboard occupied = board.getOccupied();

        bool generatesRook = (pieceType == Piece::WhiteRook || pieceType == Piece::BlackRook ||
                              pieceType == Piece::WhiteQueen || pieceType == Piece::BlackQueen);
        bool generatesBishop = (pieceType == Piece::WhiteBishop || pieceType == Piece::BlackBishop ||
                                pieceType == Piece::WhiteQueen || pieceType == Piece::BlackQueen);

        while (pieces) {
            int fromSquareIdx = BitboardOps::popLSB(pieces);
            Square fromSquare = static_cast<Square>(fromSquareIdx);

            Bitboard attacks = 0ULL;
            if (generatesRook)   attacks |= Attacks::rookAttacksBB(fromSquare, occupied);
            if (generatesBishop) attacks |= Attacks::bishopAttacksBB(fromSquare, occupied);

            attacks &= ~friendlyPieces;

            while (attacks) {
                int targetIdx = BitboardOps::popLSB(attacks);
                Square targetSquare = static_cast<Square>(targetIdx);
                moveList.add(fromSquare, targetSquare);
            }
        }
    }

    // Fast Knight Generation via Attack Tables
    void generateKnightMoves(const Board& board, MoveList& moveList, Color side) {
        Piece knightPiece = (side == Color::White) ? Piece::WhiteKnight : Piece::BlackKnight;
        Bitboard knights = board.getPieces(knightPiece);
        Bitboard friendlyPieces = (side == Color::White) ? board.getWhitePieces() : board.getBlackPieces();

        while (knights) {
            int fromSquareIdx = BitboardOps::popLSB(knights);
            Square fromSquare = static_cast<Square>(fromSquareIdx);

            Bitboard attacks = Attacks::knightAttacks[fromSquareIdx] & ~friendlyPieces;

            while (attacks) {
                int targetIdx = BitboardOps::popLSB(attacks);
                moveList.add(fromSquare, static_cast<Square>(targetIdx));
            }
        }
    }

    void generateCastlingMoves(const Board& board, MoveList& moveList, Color side) {
        uint8_t rights = board.getCastlingRights();
        Bitboard occupied = board.getOccupied();
        Color enemySide = (side == Color::White) ? Color::Black : Color::White;

        if (side == Color::White) {
            if (board.pieceAt(Square::E1) == Piece::WhiteKing) {
                if (!board.isSquareAttacked(Square::E1, enemySide)) {
                    if ((rights & WhiteKingSide) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::F1)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::G1)) &&
                        !board.isSquareAttacked(Square::F1, enemySide) &&
                        !board.isSquareAttacked(Square::G1, enemySide)) {
                        moveList.add(Square::E1, Square::G1);
                    }
                    if ((rights & WhiteQueenSide) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::D1)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::C1)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::B1)) &&
                        !board.isSquareAttacked(Square::D1, enemySide) &&
                        !board.isSquareAttacked(Square::C1, enemySide)) {
                        moveList.add(Square::E1, Square::C1);
                    }
                }
            }
        } else {
            if (board.pieceAt(Square::E8) == Piece::BlackKing) {
                if (!board.isSquareAttacked(Square::E8, enemySide)) {
                    if ((rights & BlackKingSide) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::F8)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::G8)) &&
                        !board.isSquareAttacked(Square::F8, enemySide) &&
                        !board.isSquareAttacked(Square::G8, enemySide)) {
                        moveList.add(Square::E8, Square::G8);
                    }
                    if ((rights & BlackQueenSide) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::D8)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::C8)) &&
                        !BitboardOps::getBit(occupied, static_cast<int>(Square::B8)) &&
                        !board.isSquareAttacked(Square::D8, enemySide) &&
                        !board.isSquareAttacked(Square::C8, enemySide)) {
                        moveList.add(Square::E8, Square::C8);
                    }
                }
            }
        }
    }

    // Fast King Generation via Attack Tables
    void generateKingMoves(const Board& board, MoveList& moveList, Color side) {
        Piece kingPiece = (side == Color::White) ? Piece::WhiteKing : Piece::BlackKing;
        Bitboard kings = board.getPieces(kingPiece);
        Bitboard friendlyPieces = (side == Color::White) ? board.getWhitePieces() : board.getBlackPieces();

        while (kings) {
            int fromSquareIdx = BitboardOps::popLSB(kings);
            Square fromSquare = static_cast<Square>(fromSquareIdx);

            Bitboard attacks = Attacks::kingAttacks[fromSquareIdx] & ~friendlyPieces;

            while (attacks) {
                int targetIdx = BitboardOps::popLSB(attacks);
                moveList.add(fromSquare, static_cast<Square>(targetIdx));
            }
        }

        generateCastlingMoves(board, moveList, side);
    }

    void generatePawnMoves(const Board& board, MoveList& moveList, Color side) {
        Piece pawnPiece = (side == Color::White) ? Piece::WhitePawn : Piece::BlackPawn;
        Bitboard pawns = board.getPieces(pawnPiece);
        Bitboard occupied = board.getOccupied();
        Bitboard enemyPieces = (side == Color::White) ? board.getBlackPieces() : board.getWhitePieces();

        int startRank = (side == Color::White) ? 1 : 6;
        int promotionRank = (side == Color::White) ? 7 : 0;

        auto addPawnMove = [&](Square from, Square to, int targetRank) {
            if (targetRank == promotionRank) {
                if (side == Color::White) {
                    moveList.add(from, to, Piece::WhiteQueen);
                    moveList.add(from, to, Piece::WhiteRook);
                    moveList.add(from, to, Piece::WhiteBishop);
                    moveList.add(from, to, Piece::WhiteKnight);
                } else {
                    moveList.add(from, to, Piece::BlackQueen);
                    moveList.add(from, to, Piece::BlackRook);
                    moveList.add(from, to, Piece::BlackBishop);
                    moveList.add(from, to, Piece::BlackKnight);
                }
            } else {
                moveList.add(from, to);
            }
        };

        while (pawns) {
            int fromSquareIdx = BitboardOps::popLSB(pawns);

            Square fromSquare = static_cast<Square>(fromSquareIdx);
            int rank = fromSquareIdx / 8;
            int file = fromSquareIdx % 8;

            // Single Push
            int targetRank = rank + (side == Color::White ? 1 : -1);
            if (targetRank >= 0 && targetRank < 8) {
                int targetIdx = targetRank * 8 + file;
                Square targetSquare = static_cast<Square>(targetIdx);

                if (!BitboardOps::getBit(occupied, targetIdx)) {
                    addPawnMove(fromSquare, targetSquare, targetRank);

                    // Double Push
                    if (rank == startRank) {
                        int doubleTargetRank = rank + (side == Color::White ? 2 : -2);
                        int doubleTargetIdx = doubleTargetRank * 8 + file;
                        Square doubleTargetSquare = static_cast<Square>(doubleTargetIdx);

                        if (!BitboardOps::getBit(occupied, doubleTargetIdx) &&
                            !BitboardOps::getBit(occupied, targetIdx)) {
                            moveList.add(fromSquare, doubleTargetSquare);
                        }
                    }
                }
            }

            // Captures
            int captureFiles[2] = { file - 1, file + 1 };
            for (int cf : captureFiles) {
                if (cf >= 0 && cf < 8 && targetRank >= 0 && targetRank < 8) {
                    int targetIdx = targetRank * 8 + cf;
                    Square targetSquare = static_cast<Square>(targetIdx);

                    if (BitboardOps::getBit(enemyPieces, targetIdx) ||
                        (board.getEnPassantSquare() != Square::None && targetSquare == board.getEnPassantSquare())) {
                        addPawnMove(fromSquare, targetSquare, targetRank);
                    }
                }
            }
        }
    }

    MoveList generateMoves(const Board& board) {
        MoveList moveList;
        Color side = board.getSideToMove();

        generateKnightMoves(board, moveList, side);
        generateKingMoves(board, moveList, side);
        generatePawnMoves(board, moveList, side);

        Piece rook = (side == Color::White) ? Piece::WhiteRook : Piece::BlackRook;
        generateSliderMoves(board, moveList, side, rook);

        Piece bishop = (side == Color::White) ? Piece::WhiteBishop : Piece::BlackBishop;
        generateSliderMoves(board, moveList, side, bishop);

        Piece queen = (side == Color::White) ? Piece::WhiteQueen : Piece::BlackQueen;
        generateSliderMoves(board, moveList, side, queen);

        return moveList;
    }

    MoveList generateLegalMoves(Board& board) {
        MoveList pseudoMoves = MoveGen::generateMoves(board);
        MoveList legalMoves;
        Color side = board.getSideToMove();
        Color enemySide = (side == Color::White) ? Color::Black : Color::White;
        Piece kingPiece = (side == Color::White) ? Piece::WhiteKing : Piece::BlackKing;

        for (int i = 0; i < pseudoMoves.count; i++) {
            Move move = pseudoMoves.moves[i];
            Undo undo;

            if (!board.makeMove(move, undo)) continue;

            // directly grab the king square from the updated board state
            Bitboard kingBitboard = board.getPieces(kingPiece);
            if (kingBitboard != 0) {
                Square kingSquare = static_cast<Square>(BitboardOps::getLSB(kingBitboard));
                if (!board.isSquareAttacked(kingSquare, enemySide)) {
                    legalMoves.moves[legalMoves.count++] = move;
                }
            }

            board.unmakeMove(move, undo);
        }

        return legalMoves;
    }
}
