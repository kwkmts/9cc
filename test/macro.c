void assert(int expected, int actual, char *code);
int printf(const char *, ...);
int strcmp(const char *, const char *);

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

#if 1
    x = 4;
#else
    x = 5;
#endif
    assert(4, x, "x");

#if 0
    x = 4;
#else
    x = 5;
#endif
    assert(5, x, "x");

#if 0
    x = 10;
#elif 0
    x = 20;
#elif 1
    x = 30;
#elif 2
    x = 40;
#endif
    assert(30, x, "x");

#if 1
    x = 10;
#elif 2
    x = 20;
#elif 3
    x = 30;
#endif
    assert(10, x, "x");

#if 0
    x = 1;
#elif 1
#if 1
    x = 2;
#else
    x = 3;
#endif
#else
    x = 5;
#endif
    assert(2, x, "x");

#define M1 3
    assert(3, M1, "M1");
#define M1 3 + 4 +
    assert(12, M1 5, "5");

#define M2() 1
    assert(1, M2(), "M2()");

#define M3(x, y) x + y
    assert(7, M3(3, 4), "M3(3, 4)");
    assert(4, M3(, 4), "M3(, 4)");

#define M4(x, y) x *y
    assert(24, M4(3 + 4, 4 + 5), "M4(3+4, 4+5)");

#define M4(x, y) (x) * (y)
    assert(63, M4(3 + 4, 4 + 5), "M4(3+4, 4+5)");

#define M5(x) (x) * (x) * (x)
    assert(64, M5(4), "M5(4)");

    x = 4;
#ifdef M6
    x = 5;
#endif
    assert(4, x, "x");

    x = 4;
#define M6
#ifdef M6
    x = 5;
#endif
    assert(5, x, "x");

    x = 4;
#ifndef M7
    x = 5;
#endif
    assert(5, x, "x");

    x = 4;
#define M7
#ifndef M7
    x = 5;
#endif
    assert(4, x, "x");

#ifdef M7
    x = 6;
#else
    x = 7;
#endif
    assert(6, x, "x");

#ifndef M7
    x = 6;
#else
    x = 7;
#endif
    assert(7, x, "x");

#undef M7
#ifndef M7
    x = 6;
#else
    x = 7;
#endif
    assert(6, x, "x");

    // clang-format off
#define M8(x) #x
    assert(0, strcmp(M8(a!b  `"def"g), "a!b `\"def\"g"), "strcmp(M8( a!b  `\"def\"g), \"a!b `\\\"def\\\"g\")");
    // clang-format on

#define M9 2##3
    assert(23, M9, "M9");

#define paste(x,y) x##y
    assert(15, paste(1,5), "paste(1,5)");
    assert(255, paste(0,xff), "paste(0,xff)");
    assert(3, ({ int foobar=3; paste(foo,bar); }), "({ int foobar=3; paste(foo,bar); })");
    assert(5, paste(5,), "paste(5,)");
    assert(5, paste(,5), "paste(,5)");

#define i 5
    assert(101, ({ int i3=100; paste(1+i,3); }), "({ int i3=100; paste(1+i,3); })");
#undef i

#define paste2(x) x##5
    assert(26, paste2(1+2), "paste2(1+2)");

#define paste3(x) 2##x
    assert(23, paste3(1+2), "paste3(1+2)");

#define paste4(x, y, z) x##y##z
    assert(123, paste4(1,2,3), "paste4(1,2,3)");

    return 0;
}
