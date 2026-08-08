#include "uci.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

#include "../board/board.h"
#include "../movegen/movegen.h"
#include "../movegen/move.h"
#include "../search/searcher.h"

namespace {
    unsigned long long perft(int depth, Board& board) {
        if (depth == 0) return 1ULL;

        MoveList pseudoMoves = MoveGen::generateMoves(board);
        unsigned long long nodes = 0;

        for (int i = 0; i < pseudoMoves.count; ++i) {
            Undo undo;
            if (!board.makeMove(pseudoMoves.moves[i], undo)) {
                continue;
            }

            nodes += perft(depth - 1, board);

            board.unmakeMove(pseudoMoves.moves[i], undo);
        }
        return nodes;
    }
}

namespace Uci {
    void loop() {
        Board board;
        board.reset();
        int hashSizeMb = 16;

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string token;
            ss >> token;

            if (token == "uci") {
                std::cout << "id name NeuralDuck\n"
                          << "id author carty\n"
                          << "option name Hash type spin default 16 min 1 max 1024\n"
                          << "option name Threads type spin default 1 min 1 max 1\n"
                          << "uciok\n" << std::flush;
            }
            else if (token == "isready") {
                std::cout << "readyok\n" << std::flush;
            }
            else if (token == "setoption") {
                std::string name, value, tok;
                while (ss >> tok) {
                    if (tok == "value") break;
                    if (tok != "name") name += (name.empty() ? "" : " ") + tok;
                }
                ss >> value;
                if (name == "Hash") {
                    try { hashSizeMb = std::stoi(value); } catch (...) {}
                }
            }
            else if (token == "ucinewgame") {
                board.reset();
            }
            else if (token == "perft") {
                int depth = 1;
                ss >> depth;

                auto start = std::chrono::high_resolution_clock::now();
                unsigned long long totalNodes = perft(depth, board);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                std::cout << "--- Perft Results ---\n"
                          << "Depth: " << depth << "\n"
                          << "Nodes: " << totalNodes << "\n"
                          << "Time:  " << duration << " ms\n"
                          << "---------------------\n" << std::flush;
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
                            break;
                        }
                    }
                    board.loadFEN(fenParts);

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

                    int allocated = (remainingTime / 30) + (increment / 2);

                    if (allocated > remainingTime - 50) allocated = remainingTime - 50;
                    if (allocated < 10) allocated = 10;

                    limits.moveTimeMs = allocated;
                }
                else {
                    limits.maxDepth = 10;
                }

                Searcher searcher(hashSizeMb);
                Move bestMove = searcher.findBestMove(board, limits);

                if (bestMove != Move()) {
                    std::cout << "bestmove " << moveToUci(bestMove) << "\n" << std::flush;
                } else {
                    std::cout << "bestmove 0000\n" << std::flush;
                }
            }
            else if (token == "quit") {
                break;
            }
        }
    }
}
