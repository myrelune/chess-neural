#include "movegen.h"
#include "../board/bitboard.h"
#include "../attacks/attacks.h"

namespace MoveGen {

    // Fast Slider Generation via Magic Bitboards
    inline void generateSliderMoves(const Board& board, MoveList& moveList, Color side) {
        Bitboard friendlyPieces = (side == Color::White) ? board.getWhitePieces() : board.getBlackPieces();
        Bitboard occupied = board.getOccupied();

        // Rooks
        Bitboard rooks = (side == Color::White) ? board.getPieces(Piece::WhiteRook) : board.getPieces(Piece::BlackRook);
        while (rooks) {
            Square fromSquare = static_cast<Square>(BitboardOps::popLSB(rooks));
            Bitboard attacks = Attacks::rookAttacksBB(fromSquare, occupied) & ~friendlyPieces;
            while (attacks) {
                moveList.add(fromSquare, static_cast<Square>(BitboardOps::popLSB(attacks)));
            }
        }

        // Bishops
        Bitboard bishops = (side == Color::White) ? board.getPieces(Piece::WhiteBishop) : board.getPieces(Piece::BlackBishop);
        while (bishops) {
            Square fromSquare = static_cast<Square>(BitboardOps::popLSB(bishops));
            Bitboard attacks = Attacks::bishopAttacksBB(fromSquare, occupied) & ~friendlyPieces;
            while (attacks) {
                moveList.add(fromSquare, static_cast<Square>(BitboardOps::popLSB(attacks)));
            }
        }

        // Queens
        Bitboard queens = (side == Color::White) ? board.getPieces(Piece::WhiteQueen) : board.getPieces(Piece::BlackQueen);
        while (queens) {
            Square fromSquare = static_cast<Square>(BitboardOps::popLSB(queens));
            Bitboard attacks = (Attacks::rookAttacksBB(fromSquare, occupied) | Attacks::bishopAttacksBB(fromSquare, occupied)) & ~friendlyPieces;
            while (attacks) {
                moveList.add(fromSquare, static_cast<Square>(BitboardOps::popLSB(attacks)));
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

    constexpr Bitboard FILE_A = 0x0101010101010101ULL;
    constexpr Bitboard FILE_H = 0x8080808080808080ULL;
    constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
    constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

    void generatePawnMoves(const Board& board, MoveList& moveList, Color side) {
        Bitboard occupied = board.getOccupied();
        Square epSq = board.getEnPassantSquare();

        if (side == Color::White) {
            Bitboard pawns = board.getPieces(Piece::WhitePawn);
            Bitboard enemyPieces = board.getBlackPieces();
            Bitboard targets = enemyPieces | (epSq != Square::None ? (1ULL << static_cast<int>(epSq)) : 0ULL);

            // Single Push (+8)
            Bitboard singlePush = (pawns << 8) & ~occupied;
            Bitboard singlePushNoPromo = singlePush & ~RANK_8;
            while (singlePushNoPromo) {
                int to = BitboardOps::popLSB(singlePushNoPromo);
                moveList.add(static_cast<Square>(to - 8), static_cast<Square>(to));
            }
            Bitboard singlePushPromo = singlePush & RANK_8;
            while (singlePushPromo) {
                int to = BitboardOps::popLSB(singlePushPromo);
                Square from = static_cast<Square>(to - 8);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::WhiteQueen);
                moveList.add(from, target, Piece::WhiteRook);
                moveList.add(from, target, Piece::WhiteBishop);
                moveList.add(from, target, Piece::WhiteKnight);
            }

            // Double Push (+16 from Rank 2)
            Bitboard doublePush = ((singlePush & 0x0000000000FF0000ULL) << 8) & ~occupied;
            while (doublePush) {
                int to = BitboardOps::popLSB(doublePush);
                moveList.add(static_cast<Square>(to - 16), static_cast<Square>(to));
            }

            // Captures Left (NW = +7)
            Bitboard capLeft = ((pawns & ~FILE_A) << 7) & targets;
            Bitboard capLeftNoPromo = capLeft & ~RANK_8;
            while (capLeftNoPromo) {
                int to = BitboardOps::popLSB(capLeftNoPromo);
                moveList.add(static_cast<Square>(to - 7), static_cast<Square>(to));
            }
            Bitboard capLeftPromo = capLeft & RANK_8;
            while (capLeftPromo) {
                int to = BitboardOps::popLSB(capLeftPromo);
                Square from = static_cast<Square>(to - 7);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::WhiteQueen);
                moveList.add(from, target, Piece::WhiteRook);
                moveList.add(from, target, Piece::WhiteBishop);
                moveList.add(from, target, Piece::WhiteKnight);
            }

            // Captures Right (NE = +9)
            Bitboard capRight = ((pawns & ~FILE_H) << 9) & targets;
            Bitboard capRightNoPromo = capRight & ~RANK_8;
            while (capRightNoPromo) {
                int to = BitboardOps::popLSB(capRightNoPromo);
                moveList.add(static_cast<Square>(to - 9), static_cast<Square>(to));
            }
            Bitboard capRightPromo = capRight & RANK_8;
            while (capRightPromo) {
                int to = BitboardOps::popLSB(capRightPromo);
                Square from = static_cast<Square>(to - 9);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::WhiteQueen);
                moveList.add(from, target, Piece::WhiteRook);
                moveList.add(from, target, Piece::WhiteBishop);
                moveList.add(from, target, Piece::WhiteKnight);
            }
        } else {
            Bitboard pawns = board.getPieces(Piece::BlackPawn);
            Bitboard enemyPieces = board.getWhitePieces();
            Bitboard targets = enemyPieces | (epSq != Square::None ? (1ULL << static_cast<int>(epSq)) : 0ULL);

            // Single Push (-8)
            Bitboard singlePush = (pawns >> 8) & ~occupied;
            Bitboard singlePushNoPromo = singlePush & ~RANK_1;
            while (singlePushNoPromo) {
                int to = BitboardOps::popLSB(singlePushNoPromo);
                moveList.add(static_cast<Square>(to + 8), static_cast<Square>(to));
            }
            Bitboard singlePushPromo = singlePush & RANK_1;
            while (singlePushPromo) {
                int to = BitboardOps::popLSB(singlePushPromo);
                Square from = static_cast<Square>(to + 8);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::BlackQueen);
                moveList.add(from, target, Piece::BlackRook);
                moveList.add(from, target, Piece::BlackBishop);
                moveList.add(from, target, Piece::BlackKnight);
            }

            // Double Push (-16 from Rank 7)
            Bitboard doublePush = ((singlePush & 0x0000FF0000000000ULL) >> 8) & ~occupied;
            while (doublePush) {
                int to = BitboardOps::popLSB(doublePush);
                moveList.add(static_cast<Square>(to + 16), static_cast<Square>(to));
            }

            // Captures Left (SW = -9)
            Bitboard capLeft = ((pawns & ~FILE_A) >> 9) & targets;
            Bitboard capLeftNoPromo = capLeft & ~RANK_1;
            while (capLeftNoPromo) {
                int to = BitboardOps::popLSB(capLeftNoPromo);
                moveList.add(static_cast<Square>(to + 9), static_cast<Square>(to));
            }
            Bitboard capLeftPromo = capLeft & RANK_1;
            while (capLeftPromo) {
                int to = BitboardOps::popLSB(capLeftPromo);
                Square from = static_cast<Square>(to + 9);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::BlackQueen);
                moveList.add(from, target, Piece::BlackRook);
                moveList.add(from, target, Piece::BlackBishop);
                moveList.add(from, target, Piece::BlackKnight);
            }

            // Captures Right (SE = -7)
            Bitboard capRight = ((pawns & ~FILE_H) >> 7) & targets;
            Bitboard capRightNoPromo = capRight & ~RANK_1;
            while (capRightNoPromo) {
                int to = BitboardOps::popLSB(capRightNoPromo);
                moveList.add(static_cast<Square>(to + 7), static_cast<Square>(to));
            }
            Bitboard capRightPromo = capRight & RANK_1;
            while (capRightPromo) {
                int to = BitboardOps::popLSB(capRightPromo);
                Square from = static_cast<Square>(to + 7);
                Square target = static_cast<Square>(to);
                moveList.add(from, target, Piece::BlackQueen);
                moveList.add(from, target, Piece::BlackRook);
                moveList.add(from, target, Piece::BlackBishop);
                moveList.add(from, target, Piece::BlackKnight);
            }
        }
    }

    void generateCaptureMoves(const Board& board, MoveList& moveList) {
        Color side = board.getSideToMove();
        Bitboard enemyPieces = (side == Color::White) ? board.getBlackPieces() : board.getWhitePieces();
        Bitboard occupied = board.getOccupied();

        Piece knightPiece = (side == Color::White) ? Piece::WhiteKnight : Piece::BlackKnight;
        Bitboard knights = board.getPieces(knightPiece);
        while (knights) {
            int fromIdx = BitboardOps::popLSB(knights);

            Bitboard attacks = Attacks::knightAttacks[fromIdx] & enemyPieces;
            while (attacks) {
                int targetIdx = BitboardOps::popLSB(attacks);
                moveList.add(static_cast<Square>(fromIdx), static_cast<Square>(targetIdx));
            }
        }

        Piece kingPiece = (side == Color::White) ? Piece::WhiteKing : Piece::BlackKing;
        Bitboard kings = board.getPieces(kingPiece);
        if (kings) {
            int fromIdx = BitboardOps::popLSB(kings);
            Bitboard attacks = Attacks::kingAttacks[fromIdx] & enemyPieces;
            while (attacks) {
                int targetIdx = BitboardOps::popLSB(attacks);
                moveList.add(static_cast<Square>(fromIdx), static_cast<Square>(targetIdx));
            }
        }

        Piece pawnPiece = (side == Color::White) ? Piece::WhitePawn : Piece::BlackPawn;
        Bitboard pawns = board.getPieces(pawnPiece);
        int promotionRank = (side == Color::White) ? 7 : 0;

        while (pawns) {
            int fromIdx = BitboardOps::popLSB(pawns);
            Square fromSquare = static_cast<Square>(fromIdx);
            int rank = fromIdx / 8;
            int file = fromIdx % 8;
            int targetRank = rank + (side == Color::White ? 1 : -1);

            if (targetRank >= 0 && targetRank < 8) {

                // Check Promotions
                if (targetRank == promotionRank) {
                    int pushIdx = targetRank * 8 + file;
                    if (!BitboardOps::getBit(occupied, pushIdx)) {
                        moveList.add(fromSquare, static_cast<Square>(pushIdx),
                                        (side == Color::White) ? Piece::WhiteQueen : Piece::BlackQueen);
                    }
                }

                // Check Captures
                int captureFiles[2] = { file - 1, file + 1 };
                for (int cf : captureFiles) {
                    if (cf >= 0 && cf < 8) {
                        int targetIdx = targetRank * 8 + cf;
                        Square targetSquare = static_cast<Square>(targetIdx);

                        if (BitboardOps::getBit(enemyPieces, targetIdx) ||
                            (board.getEnPassantSquare() != Square::None && targetSquare == board.getEnPassantSquare())) {

                            if (targetRank == promotionRank) {
                                moveList.add(fromSquare, targetSquare, (side == Color::White) ? Piece::WhiteQueen : Piece::BlackQueen);
                            } else {
                                moveList.add(fromSquare, targetSquare);
                            }
                        }
                    }
                }
            }
        }

        Piece types[3] = {
            (side == Color::White) ? Piece::WhiteBishop : Piece::BlackBishop,
            (side == Color::White) ? Piece::WhiteRook : Piece::BlackRook,
            (side == Color::White) ? Piece::WhiteQueen : Piece::BlackQueen
        };

        for (Piece p : types) {
            Bitboard pieces = board.getPieces(p);
            bool isRook = (p == Piece::WhiteRook || p == Piece::BlackRook || p == Piece::WhiteQueen || p == Piece::BlackQueen);
            bool isBishop = (p == Piece::WhiteBishop || p == Piece::BlackBishop || p == Piece::WhiteQueen || p == Piece::BlackQueen);

            while (pieces) {
                int fromIdx = BitboardOps::popLSB(pieces);
                Bitboard attacks = 0ULL;

                if (isRook)   attacks |= Attacks::rookAttacksBB(static_cast<Square>(fromIdx), occupied);
                if (isBishop) attacks |= Attacks::bishopAttacksBB(static_cast<Square>(fromIdx), occupied);

                attacks &= enemyPieces;

                while (attacks) {
                    int targetIdx = BitboardOps::popLSB(attacks);
                    moveList.add(static_cast<Square>(fromIdx), static_cast<Square>(targetIdx));
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
        generateSliderMoves(board, moveList, side);

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
