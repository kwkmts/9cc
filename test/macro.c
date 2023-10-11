void assert(int expected, int actual, char *code);
int printf(const char *, ...);

#include "include1.h"

#
/**/ #

int main() {
    assert(5, include1, "include1");
    assert(7, include2, "include2");

#if 0
    printf("should not be printed!\n");
    return 1;
#endif

    int x = 0;
#if 1
    x = 1;
#if 0
    x = 2;
#if 1
    x = 3;
#endif
#endif
#endif
    assert(1, x, "x");

#define M1 3
    assert(3, M1, "M1");
#define M1 3 + 4 +
    assert(12, M1 5, "5");

    return 0;
}
