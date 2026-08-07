#include "uci/uci.h"
#include <cstdio>
#include "eval/tables.h"
#include "zobrist/zobrist.h"
#include "attacks/attacks.h"
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cout << std::unitbuf;

    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    Tables::initTables();
    Zobrist::init();
    Attacks::initAttacks();

    Uci::loop();
    return 0;
}