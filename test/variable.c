#include "test.h"

int g1;
int g2;
int g3[4];
int g4 = 42;
static int g5 = 3;
extern int ext1;
extern int *ext2;
int ext3 = 42;
const int g6;
int;
typedef void gV;

int main() {
    int l1 = 0;
    int l2 = 42;
    void *l6;
    typedef void V;

    ASSERT(0, l1);
    ASSERT(42, l2);

    ASSERT(0, g1);
    ASSERT(3, ({ g1=3; g1; }));
    ASSERT(7, ({ g1=3; g2=4; g1+g2; }));
    ASSERT(0, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[0]; }));
    ASSERT(1, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[1]; }));
    ASSERT(2, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[2]; }));
    ASSERT(3, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[3]; }));
    ASSERT(42, g4);
    ASSERT(4, sizeof(g1));
    ASSERT(16, sizeof(g3));

    ASSERT(3, ({ int a; a=3; a; }));
    ASSERT(8, ({ int a=3; int z=5; a+z; }));
    ASSERT(6, ({ int a; int b; a=b=3; a+b; }));
    ASSERT(3, ({ int foo; foo=3; foo; }));
    ASSERT(8, ({ int foo123=3; int bar=5; foo123+bar; }));

    ASSERT(3, ({ int x=3; *&x; }));
    ASSERT(3, ({ int x=3; int *y; int **z; y=&x; z=&y; **z; }));
    ASSERT(5, ({ int x=3; int y=5; *(&x-1); }));
    ASSERT(3, ({ int x=3; int y=5; *(&y+1); }));
    ASSERT(5, ({ int x=3; int y=5; *(&x+(-1)); }));
    ASSERT(5, ({ int x=3; int *y; y=&x; *y=5; x; }));
    ASSERT(7, ({ int x=3; int y=5; *(&x-1)=7; y; }));
    ASSERT(7, ({ int x=3; int y=5; *(&y+1)=7; x; }));
    ASSERT(5, ({ int x=3; (&x+2)-&x+3; }));

    ASSERT(4, ({ int x; sizeof(x); }));
    ASSERT(4, ({ int x; sizeof x; }));
    ASSERT(8, ({ int *x; sizeof(x); }));
    ASSERT(16, ({ int x[4]; sizeof(x); }));
    ASSERT(4, ({ int x=1; sizeof(x=2); }));
    ASSERT(1, ({ int x=1; sizeof(x=2); x; }));
    ASSERT(1, ({ char i; sizeof(++i); }));
    ASSERT(1, ({ char i; sizeof(i++); }));
    ASSERT(2, ({ short x; sizeof(x); }));
    ASSERT(8, ({ long x; sizeof(x); }));

    ASSERT(0, ({ _Bool x=0; x; }));
    ASSERT(1, ({ _Bool x=1; x; }));
    ASSERT(1, ({ _Bool x=2; x; }));
    ASSERT(1, (_Bool)1);
    ASSERT(1, (_Bool)2);
    ASSERT(0, (_Bool)(char)256);

    ASSERT(3, ({ int x[2]; int *y; y=&x; *y=3; *x; }));

    ASSERT(3, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; }));
    ASSERT(4, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); }));
    ASSERT(5, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); }));

    ASSERT(0, ({ int x[2][3]; int *y=x; *y=0; **x; }));
    ASSERT(1, ({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); }));
    ASSERT(2, ({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); }));
    ASSERT(3, ({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); }));
    ASSERT(4, ({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); }));
    ASSERT(5, ({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); }));

    ASSERT(3, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; }));
    ASSERT(4, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); }));
    ASSERT(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }));
    ASSERT(5, ({ int x[3]; x[0]=3; x[1]=4; *(x+2)=5; x[2]; }));
    ASSERT(3, ({ int i=0; int x[3]; x[(1-1)]=3; x[1*1]=4; x[1+1]=5; x[i]; }));

    ASSERT(0, ({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; }));
    ASSERT(1, ({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; }));
    ASSERT(2, ({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; }));
    ASSERT(3, ({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; }));
    ASSERT(4, ({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; }));
    ASSERT(5, ({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; }));

    ASSERT(1, ({ char x=1; x; }));
    ASSERT(1, ({ char x=1; char y=2; x; }));
    ASSERT(2, ({ char x=1; char y=2; y; }));

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(10, ({ char x[10]; sizeof(x); }));

    ASSERT(2, ({ int x=2; { int x=3; } x; }));
    ASSERT(2, ({ int x=2; { int x=3; } int y=4; x; }));
    ASSERT(3, ({ int x=2; { x=3; } x; }));

    ASSERT(1, ({ int x; int y; char z; char *a=&y; char *b=&z; a-b; }));
    ASSERT(7, ({ int x; char y; int z; char *a=&y; char *b=&z; a-b; }));

    ASSERT(24, ({ char *x[3]; sizeof(x); }));
    ASSERT(8, ({ char (*x)[3]; sizeof(x); }));
    ASSERT(1, ({ char (x); sizeof(x); }));
    ASSERT(3, ({ char (x)[3]; sizeof(x); }));
    ASSERT(12, ({ char (x[3])[4]; sizeof(x); }));
    ASSERT(4, ({ char (x[3])[4]; sizeof(x[0]); }));
    ASSERT(3, ({ char *x[3]; char y; x[0]=&y; y=3; x[0][0]; }));
    ASSERT(4, ({ char x[3]; char (*y)[3]=x; y[0][0]=4; y[0][0]; }));

    ASSERT(3, g5);

    ASSERT(5, ext1);
    ASSERT(5, *ext2);

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(2, ({ short int x; sizeof(x); }));
    ASSERT(2, ({ int short x; sizeof(x); }));
    ASSERT(4, ({ int x; sizeof(x); }));
    ASSERT(8, ({ long int x; sizeof(x); }));
    ASSERT(8, ({ int long x; sizeof(x); }));
    ASSERT(8, ({ long long x; sizeof(x); }));

    ASSERT(5, ({ const int x = 5; x; }));
    ASSERT(8, ({ const int x = 8; int *const y=&x; *y; }));
    ASSERT(6, ({ int const x = 6; *(int const * const)&x; }));
    ASSERT(10, ({ const struct t {char a; int b;} x={10,100}; x.a; }));
    ASSERT(2, ({ const int a[4]={1,2,3,4}; a[1];}));

    { int volatile x; }
    { volatile int x; }
    { int volatile * volatile x; }
    { int ** restrict const volatile *x; }
    { int (* volatile x); }
    { int * volatile (* restrict * const x)[3]; }

    ASSERT(6, ({ char a[2*3]; sizeof(a); }));
    ASSERT(16, ({ int a[(char)260]; sizeof(a); }));
    ASSERT(8, ({ char a[sizeof(int*)]; sizeof(a); }));

    return 0;
}
