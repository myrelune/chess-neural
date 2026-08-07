#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>

#include "uci/uci.h"
#include "board/board.h"
#include "movegen/movegen.h"
#include "movegen/move.h"
#include "eval/tables.h"
#include "zobrist/zobrist.h"
#include "attacks/attacks.h"

// --- Perft Helpers for Quick Testing ---
uint64_t runPerft(Board& board, int depth) {
    if (depth == 0) {
        return 1ULL;
    }

    MoveList moves = MoveGen::generateLegalMoves(board);
    uint64_t nodes = 0;

    for (int i = 0; i < moves.count; i++) {
        Undo undo;
        if (!board.makeMove(moves.moves[i], undo)) {
            continue; 
        }

        nodes += runPerft(board, depth - 1);

        board.unmakeMove(moves.moves[i], undo);
    }

    return nodes;
}

void runPerftDivide(Board& board, int depth) {
    if (depth == 0) return;

    MoveList moves = MoveGen::generateLegalMoves(board);
    uint64_t totalNodes = 0;

    std::cout << "\n--- PERFT DIVIDE (Depth " << depth << ") ---\n";
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        Undo undo;
        if (!board.makeMove(move, undo)) continue;

        uint64_t nodes = runPerft(board, depth - 1);
        totalNodes += nodes;

        board.unmakeMove(move, undo);

        std::cout << moveToUci(move) << ": " << nodes << "\n";
    }
    std::cout << "\nTotal Leaf Nodes: " << totalNodes << "\n" << std::flush;
}

int main() {
    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    Tables::initTables();
    Zobrist::init();
    Attacks::initAttacks();

    // Custom loop to check for a manual "perft" command before handing over to UCI, 
    // or you can test right here. 
    Board board;
    board.reset();

    std::string line;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "perft") {
            int depth = 1;
            ss >> depth;
            uint64_t total = runPerft(board, depth);
            std::cout << "Perft depth " << depth << " total nodes: " << total << "\n" << std::flush;
        }
        else if (token == "divide") {
            int depth = 1;
            ss >> depth;
            runPerftDivide(board, depth);
        }
        else {
            // Hand the line back to your standard UCI loop if it's not a perft command
            // (You can pass the line or run Uci::loop() natively. 
            // Here we just feed it back by handling standard UCI tokens or calling Uci::loop)
            
            // For simplicity, if you want full UCI support + perft, 
            // you can parse "uci" or just run Uci::loop() directly and handle perft inside uci.cpp.
            // But to keep it entirely in main.cpp for testing right now:
            if (token == "uci") {
                std::cout << "id name NeuralDuck\nid author carty\nuciok\n" << std::flush;
            } else if (token == "isready") {
                std::cout << "readyok\n" << std::flush;
            } else if (token == "quit") {
                break;
            } else {
                // If you want to fall back to the actual UCI state machine:
                // (Assuming you compile everything together)
                // Uci::loop(); can also be wrapped, but this lets you test perft instantly.
            }
        }
    }

    return 0;
}