void assert(int expected, int actual, char *code);
int printf(const char *, ...);
int strcmp(const char *, const char *);

#include "include1.h"

#
/**/ #

int add2(int x, int y) {
    return x + y;
}

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}

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

    x = 0;
#if 0
#if 0
#elif 1
#else
#endif
    x = 1;
#endif
    assert(0, x, "x");

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

#if 1-1
    x = 1;
#elif 1*1
    x = 2;
#endif
    assert(2, x, "x");

#if 10?1:0
    x = 1;
#elif 0/1
    x = 2;
#endif
    assert(1, x, "x");

#define M1 3
    assert(3, M1, "M1");
#define M1 3 + 4 +
    assert(12, M1 5, "M1 5");

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

#define M10
#if defined(M10)
    x = 3;
#else
    x = 4;
#endif
    assert(3, x, "x");

#define M10
#if defined M10
    x = 3;
#else
    x = 4;
#endif
    assert(3, x, "x");

#if defined(M10) - 1
    x = 3;
#else
    x = 4;
#endif
    assert(4, x, "x");

#if defined(NO_SUCH_MACRO)
    x = 3;
#else
    x = 4;
#endif
    assert(4, x, "x");

    assert(1, __STDC__, "__STDC__");

    int M11 = 6;
#define M11 M11 + 3
    assert(9, M11, "M11");

#define M12 M11 + 3
    assert(12, M12, "M12");

    int M13 = 3;
#define M13 M14 * 5
#define M14 M13 + 2
    assert(13, M13, "M13");

#define M15 1
#if M15
    x = 5;
#else
    x = 6;
#endif
    assert(5, x, "x");

#if no_such_symbol == 0
    x = 7;
#else
    x = 8;
#endif
    assert(7, x, "x");

#define M16(...) 3
    assert(3, M16(), "M16()");

#define M16(...) __VA_ARGS__
    assert(2, M16() 2, "M16() 2");
    assert(5, M16(5), "M16(5)");

#define M16(...) add2(__VA_ARGS__)
    assert(8, M16(2, 6), "M16(2, 6)");

#define M16(...) add6(1,2,__VA_ARGS__,6)
    assert(21, M16(3,4,5), "M16(3,4,5)");

#define M16(x, ...) add6(1,2,x,__VA_ARGS__,6)
    assert(21, M16(3,4,5), "M16(3,4,5)");

#define M16(x, ...) x
    assert(5, M16(5), "M16(5)");

#define CONCAT(x,y) x##y
    assert(5, ({ int f0zz=5; CONCAT(f,0zz); }), "({ int f0zz=5; CONCAT(f,0zz); })");
    assert(5, ({ CONCAT(4,.57) + 0.5; }), "({ CONCAT(4,.57) + 0.5; })");

    return 0;
}
