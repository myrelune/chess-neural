#include "uci.h"
#include <iostream>
#include <sstream>
#include <string>

#include "../board/board.h"
#include "../movegen/movegen.h"
#include "../movegen/move.h"
#include "../search/searcher.h"

namespace Uci {
    void loop() {
        Board board;
        board.reset();

        std::string line;
        while (std::getline(std::cin, line)) {
            std::stringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "uci") {
                std::cout << "id name NeuralDuck\n"
                          << "id author carty\n"
                          << "uciok\n" << std::flush;
            }
            else if (token == "isready") {
                std::cout << "readyok\n" << std::flush;
            }
            else if (token == "ucinewgame") {
                board.reset();
            }
            else if (token == "position") {
                std::string subtoken;
                ss >> subtoken;

                if (subtoken == "startpos") {
                    board.reset();

                    std::string nextToken;
                    if (ss >> nextToken && nextToken == "moves") {
                        std::string moveStr;
                        while (ss >> moveStr) {
                            MoveList legalMoves = MoveGen::generateLegalMoves(board);
                            bool found = false;
                            for (int i = 0; i < legalMoves.count; ++i) {
                                const Move& move = legalMoves.moves[i];
                                if (moveToUci(move) == moveStr) {
                                    Undo undo;
                                    board.makeMove(move, undo);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) break;
                        }
                    }
                }
                else if (subtoken == "fen") {
                    std::string fenParts;
                    std::string nextToken;
                    int fieldCount = 0;

                    while (ss >> nextToken) {
                        if (nextToken == "moves") break;
                        fenParts += nextToken + " ";
                        fieldCount++;
                        if (fieldCount == 6) {
                            // Next token in stream might be "moves"
                            break;
                        }
                    }
                    board.loadFEN(fenParts);

                    // If we stopped because of fieldCount == 6, check if the next token is "moves"
                    if (nextToken != "moves") {
                        ss >> nextToken;
                    }

                    if (nextToken == "moves") {
                        std::string moveStr;
                        while (ss >> moveStr) {
                            MoveList legalMoves = MoveGen::generateLegalMoves(board);
                            bool found = false;
                            for (int i = 0; i < legalMoves.count; ++i) {
                                const Move& move = legalMoves.moves[i];
                                if (moveToUci(move) == moveStr) {
                                    Undo undo;
                                    board.makeMove(move, undo);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) break;
                        }
                    }
                }
            }
            else if (token == "go") {
                std::string param;
                int wtime = -1, btime = -1, winc = 0, binc = 0, movetime = -1, depth = -1;

                while (ss >> param) {
                    if (param == "wtime") ss >> wtime;
                    else if (param == "btime") ss >> btime;
                    else if (param == "winc") ss >> winc;
                    else if (param == "binc") ss >> binc;
                    else if (param == "movetime") ss >> movetime;
                    else if (param == "depth") ss >> depth;
                }

                SearchLimits limits;

                if (depth > 0) {
                    limits.maxDepth = depth;
                } 
                else if (movetime > 0) {
                    limits.moveTimeMs = movetime;
                } 
                else if (wtime > 0 || btime > 0) {
                    Color side = board.getSideToMove();
                    int remainingTime = (side == Color::White) ? wtime : btime;
                    int increment = (side == Color::White) ? winc : binc;

                    // Basic time management: target ~30 moves remaining + half increment
                    int allocated = (remainingTime / 30) + (increment / 2);
                    
                    // Safety bounds
                    if (allocated > remainingTime - 50) allocated = remainingTime - 50;
                    if (allocated < 10) allocated = 10;

                    limits.moveTimeMs = allocated;
                } 
                else {
                    // Fallback default search depth if 'go' is called raw
                    limits.maxDepth = 6;
                }

                Searcher searcher;
                Move bestMove = searcher.findBestMove(board, limits);

                if (bestMove != Move()) {
                    std::cout << "bestmove " << moveToUci(bestMove) << "\n" << std::flush;
                } else {
                    std::cout << "bestmove 0000\n" << std::flush;
                }
            }
        }
    }
}
