#include "uci.h"
#include <iostream>
#include <sstream>
#include <string>
#include "../board/board.h"
#include "../movegen/movegen.h"
#include "../movegen/move.h"

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

                    while (ss >> nextToken && nextToken != "moves") {
                        fenParts += nextToken + " ";
                        fieldCount++;
                        if (fieldCount == 6) break;
                    }
                    board.loadFEN(fenParts);

                    if (nextToken == "moves" || (ss >> nextToken && nextToken == "moves")) {
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
                    else if (param == "infinite") { }
                }

                MoveList legalMoves = MoveGen::generateLegalMoves(board);

                if (legalMoves.count > 0) {
                    std::cout << "bestmove " << moveToUci(legalMoves.moves[0]) << "\n" << std::flush;
                } else {
                    std::cout << "bestmove 0000\n" << std::flush;
                }
            }
            else if (token == "stop") {
            }
            else if (token == "quit") {
                break;
            }
        }
    }
}
