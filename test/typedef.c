#include "test.h"

typedef int MyInt;
typedef int MyInt2[4];

int main() {
    ASSERT(1, ({ typedef int t; t x=1; x; }));
    ASSERT(1, ({ typedef struct {int a;} t; t x; x.a=1; x.a; }));
    ASSERT(2, ({ typedef struct {int a;} t; { typedef int t; } t x; x.a=2; x.a; }));
    ASSERT(3, ({ MyInt x=3; x; }));
    ASSERT(16, ({ MyInt2 x; sizeof(x); }));

    ASSERT(-1, ({ typedef short T; T x = 65535; (int)x; }));
    ASSERT(65535, ({ typedef unsigned short T; T x = 65535; (int)x; }));

    return 0;
}
