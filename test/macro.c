void assert(int expected, int actual, char *code);

#include "include1.h"

#
/**/ #

int main() {
    assert(5, include1, "include1");
    assert(7, include2, "include2");

    return 0;
}
