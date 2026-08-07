// #include "uci/uci.h"
// #include <cstdio>
// #include "eval/tables.h"

// int main() {

//     std::setbuf(stdout, NULL);
//     std::setbuf(stdin, NULL);

//     Tables::initTables();

//     Uci::loop();
//     return 0;
// }
#include "board/board.h"
#include "eval/evaluate.h"
#include "eval/tables.h"
#include <iostream>

int main()
{
    Tables::initTables();

    Board board;

    board.loadFEN("r1b1kb1r/pppppppp/PQPPPQBN/2P5/2q5/2p3p1/PPPnqnPP/RNB1K2R w KQkq - 0 1");

    board.printBoard();
    int score = Evaluate::evaluate(board);

    std::cout << "Evaluation: " << score << "\n";

    return 0;
}
