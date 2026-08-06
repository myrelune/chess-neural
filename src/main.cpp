#include "uci/uci.h"
#include <cstdio>

int main() {

    std::setbuf(stdout, NULL);
    std::setbuf(stdin, NULL);

    Uci::loop();
    return 0;
}
