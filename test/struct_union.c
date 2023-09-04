#include "test.h"

struct gT;
union gU;
typedef struct gT gT;
typedef union gU gU;

gT extT;
gU extU;

struct gT {int a; int b;};
union gU {int a; int b;};

typedef struct {
    int a;
    struct {
        int b;
        int c;
    };
} gT2;

typedef struct {
    int a;
    union {
        int b;
        int c;
    };
} gT3;

int main() {
    struct T;
    union U;
    typedef struct T T;
    typedef union U U;
    struct T {int a; int b;};
    union U {int a; int b;};

    ASSERT(1, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; }));
    ASSERT(2, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; }));
    ASSERT(1, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; }));
    ASSERT(2, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.b; }));
    ASSERT(3, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; }));

    ASSERT(0, ({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; }));
    ASSERT(1, ({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; }));
    ASSERT(2, ({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; }));
    ASSERT(3, ({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; }));

    ASSERT(6, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; }));
    ASSERT(7, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7; p[3]; }));

    ASSERT(6, ({ struct {struct {char b;} a;} x; x.a.b=6; x.a.b; }));

    ASSERT(4, ({ struct {int a;} x; sizeof(x); }));
    ASSERT(8, ({ struct {int a; int b;} x; sizeof(x); }));
    ASSERT(12, ({ struct {int a[3];} x; sizeof(x); }));
    ASSERT(16, ({ struct {int a;} x[4]; sizeof(x); }));
    ASSERT(24, ({ struct {int a[3];} x[2]; sizeof(x); }));
    ASSERT(2, ({ struct {char a; char b;} x; sizeof(x); }));
    ASSERT(0, ({ struct {} x; sizeof(x); }));
    ASSERT(8, ({ struct {char a; int b;} x; sizeof(x); }));
    ASSERT(8, ({ struct {int a; char b;} x; sizeof(x); }));
    ASSERT(4, ({ struct {char a; short b;} x; sizeof(x); }));
    ASSERT(16, ({ struct {char a; long b;} x; sizeof(x); }));

    ASSERT(8, ({ struct t {int a; int b;} x; struct t y; sizeof(y); }));
    ASSERT(8, ({ struct t {int a; int b;}; struct t y; sizeof(y); }));
    ASSERT(2, ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }));
    ASSERT(3, ({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; }));

    ASSERT(3, ({ struct t {char a;} x; struct t *y=&x; x.a=3; y->a; }));
    ASSERT(3, ({ struct t {char a;} x; struct t *y=&x; y->a=3; x.a; }));

    ASSERT(3, ({ struct t {int a; int b;}; struct t x; x.a=3; struct t y=x; y.a; }));
    ASSERT(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }));
    ASSERT(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *p=&x; struct t *q=&y; *q=*p; y.a; }));
    ASSERT(5, ({ struct t {char a; char b;} x; struct t y; x.a=5; y=x; y.a; }));


    ASSERT(8, ({ union {int a; char b[6];} x; sizeof(x); }));
    ASSERT(3, ({ union {int a; char b[4];} x; x.a=515; x.b[0]; }));
    ASSERT(2, ({ union {int a; char b[4];} x; x.a=515; x.b[1]; }));
    ASSERT(0, ({ union {int a; char b[4];} x; x.a=515; x.b[2]; }));
    ASSERT(0, ({ union {int a; char b[4];} x; x.a=515; x.b[3]; }));

    ASSERT(3, ({ union t {int a; char b[9];}; union t x; x.a=3; union t y=x; y.a; }));
    ASSERT(7, ({ union t {int a; char b[9];}; union t x; x.a=7; union t y; union t *z=&y; *z=x; y.a; }));

    ASSERT(8, ({ struct t {const int a; int const b;} x={1,2}; sizeof(x); }));

    ASSERT(0xef, ({ union {struct {unsigned char a; unsigned char b;}; long c;} x; x.c=0xbeef; x.a; }));
    ASSERT(0xbe, ({ union {struct {unsigned char a; unsigned char b;}; long c;} x; x.c=0xbeef; x.b; }));

    ASSERT(3, ({ struct {union {int a; int b;}; union {int c; int d;};} x; x.a=3; x.b; }));
    ASSERT(5, ({ struct {union {int a; int b;}; union {int c; int d;};} x; x.d=5; x.c; }));

    ASSERT(42, ({ gT2 x={64, {42}}; x.b; }));
    ASSERT(42, ({ gT3 x={64, {42}}; x.b; }));

    ASSERT(42, ({ struct gT x={}; x.a=42; }));
    ASSERT(42, ({ union gU x={}; x.a=42; }));
    ASSERT(42, ({ gT x={}; x.a=42; }));
    ASSERT(42, ({ gU x={}; x.a=42; }));

    return 0;
}
