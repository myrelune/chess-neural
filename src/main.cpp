#include "uci/uci.h"
#include <cstdio>
#include "eval/tables.h"
#include "zobrist/zobrist.h"
#include "attacks/attacks.h"

int main() {

    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    Tables::initTables();
    Zobrist::init();
    Attacks::initAttacks();

    Uci::loop();
    return 0;
}