#include "uci/uci.h"
#include <cstdio>
#include "eval/tables.h"

int main() {

    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    Tables::initTables();

    Uci::loop();
    return 0;
}