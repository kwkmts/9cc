#include "test.h"

/* ブロックコメント */

// 行コメント

int g1;
int g2;
int g3[4];
int g4 = 42;
int g5 = 12 + 34 - 5;
int g6 = 5 + 6 * 7;
int g7 = 5 * (9 - 6);
int g8 = (3 + 5) / 2;
int g9 = -10 + 20;
int g10 = - -+10;
int g11 = 0 == 1;
int g12 = 0 != 1;
int g13 = 0 < 1;
int g14 = 0 <= 1;
int g15[3] = {0, 1, 2};
int g16[2][3] = {{1, 2, 3}, {4, 5, 6}};
int g17[3] = {};
int g18[2][3]={{1, 2}};
int g19[]={0, 1, 2, 3};
int g20[][2]={{0, 1}, {2, 3}, {4, 5}};
struct {char a; int b;} g21[2] = {{1, 2}, {3, 4}};
struct {int a[2];} g22[2] = {{{1, 2}}};
union {int a; char b;} g23[2];

int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }
int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }
int sub_char(char a, char b, char c) { return a - b - c; }
int sub_short(short a, short b, short c) { return a - b - c; }
int sub_long(long a, long b, long c) { return a - b - c; }
int fib(int x) {
    if (x <= 1) return 1;
    return fib(x - 1) + fib(x - 2);
}

int main() {
    int l1 = 0;
    int l2 = 42;
    int l3[3] = {0, 1, 2};
    int l4[] = {0, 1, 2, 3};
    int l5[][2] = {{0, 1}, {2, 3}, {4, 5}};
    void *l6;

    ASSERT(0, 0);
    ASSERT(42, 42);
    ASSERT(21, 5+20-4);
    ASSERT(41, 12 + 34 - 5);
    ASSERT(47, 5 + 6 * 7);
    ASSERT(15, 5 * (9 - 6));
    ASSERT(4, (3 + 5) / 2);
    ASSERT(10, -10 + 20);
    ASSERT(10, - -10);
    ASSERT(10, - -+10);
    ASSERT(5, 17%6);

    ASSERT(0, 0 == 1);
    ASSERT(1, 42 == 42);
    ASSERT(1, 0 != 1);
    ASSERT(0, 42 != 42);

    ASSERT(1, 0 < 1);
    ASSERT(0, 1 < 1);
    ASSERT(0, 2 < 1);
    ASSERT(1, 0 <= 1);
    ASSERT(1, 1 <= 1);
    ASSERT(0, 2 <= 1);

    ASSERT(0, !1);
    ASSERT(0, !2);
    ASSERT(1, !0);

    ASSERT(1, 0||1);
    ASSERT(1, 0||(2-2)||5);
    ASSERT(0, 0||0);
    ASSERT(0, 0||(2-2));

    ASSERT(0, 0&&1);
    ASSERT(0, (2-2)&&5);
    ASSERT(1, 1&&5);

    ASSERT(-1, ~0);
    ASSERT(0, ~-1);

    ASSERT(0, 0&1);
    ASSERT(1, 3&1);
    ASSERT(3, 7&3);
    ASSERT(10, -1&10);

    ASSERT(1, 0|1);
    ASSERT(3, 2|1);

    ASSERT(0, 0^0);
    ASSERT(2, 3^1);

    ASSERT(1, 1<<0);
    ASSERT(8, 1<<3);
    ASSERT(10, 5<<1);
    ASSERT(2, 5>>1);
    ASSERT(-1, -1>>1);

    ASSERT(2, 0?1:2);
    ASSERT(1, 1?1:2);
    ASSERT(2, 0?1:1?2:3);

    1 ? 2 : (void)1;

    ASSERT(0, l1);
    ASSERT(42, l2);
    ASSERT(0, l3[0]);
    ASSERT(1, l3[1]);
    ASSERT(2, l3[2]);
    ASSERT(3, l4[3]);
    ASSERT(4, l5[2][0]);

    ASSERT(3, ({ int a; a=3; a; }));
    ASSERT(8, ({ int a=3; int z=5; a+z; }));
    ASSERT(6, ({ int a; int b; a=b=3; a+b; }));
    ASSERT(3, ({ int foo; foo=3; foo; }));
    ASSERT(8, ({ int foo123=3; int bar=5; foo123+bar; }));

    ASSERT(7, ({ int i=2; i+=5; i; }));
    ASSERT(7, ({ int i=2; i+=5; }));
    ASSERT(3, ({ int i=5; i-=2; i; }));
    ASSERT(3, ({ int i=5; i-=2; }));
    ASSERT(6, ({ int i=3; i*=2; i; }));
    ASSERT(6, ({ int i=3; i*=2; }));
    ASSERT(3, ({ int i=6; i/=2; i; }));
    ASSERT(3, ({ int i=6; i/=2; }));
    ASSERT(2, ({ int i=10; i%=4; i; }));
    ASSERT(2, ({ long i=10; i%=4; i; }));
    ASSERT(2, ({ int i=6; i&=3; i; }));
    ASSERT(7, ({ int i=6; i|=3; i; }));
    ASSERT(10, ({ int i=15; i^=5; i; }));
    ASSERT(1, ({ int i=1; i<<=0; i; }));
    ASSERT(8, ({ int i=1; i<<=3; i; }));
    ASSERT(10, ({ int i=5; i<<=1; i; }));
    ASSERT(2, ({ int i=5; i>>=1; i; }));
    ASSERT(-1, ({ int i=-1; i>>=1; i; }));

    ASSERT(3, ({ int i=2; ++i; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; ++*p; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; --*p; }));

    ASSERT(2, ({ int i=2; i++; }));
    ASSERT(2, ({ int i=2; i--; }));
    ASSERT(3, ({ int i=2; i++; i; }));
    ASSERT(1, ({ int i=2; i--; i; }));
    ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p++; }));
    ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p--; }));

    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*(p--))--; a[1]; }));

    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; }));

    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
    ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; }));
    ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; }));

    ASSERT(3, ({ 1; {2;} 3; }));
    ASSERT(5, ({ ;;; 5; }));

    ASSERT(3, ({ int x; if (0) x=2; else x=3; x; }));
    ASSERT(3, ({ int x; if (1-1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (2-1) x=2; else x=3; x; }));
    ASSERT(4, ({ int x; if (0) { x=1; x=2; x=3; } else { x=4; } x; }));
    ASSERT(3, ({ int x; if (1) { x=1; x=2; x=3; } else { x=4; } x; }));

    ASSERT(5, ({ int i=0; switch(0) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(6, ({ int i=0; switch(1) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(7, ({ int i=0; switch(2) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(0, ({ int i=0; switch(3) { case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(5, ({ int i=0; switch(0) { case 0:i=5;break; default:i=7; } i; }));
    ASSERT(7, ({ int i=0; switch(1) { case 0:i=5;break; default:i=7; } i; }));
    ASSERT(2, ({ int i=0; switch(1) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }));
    ASSERT(0, ({ int i=0; switch(3) { case 0: 0; case 1: 0; case 2: 0; i=2; } i; }));

    ASSERT(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));

    ASSERT(10, ({ int i=0; while(i<10) { i=i+1; } i; }));

    ASSERT(3, ({ int i=0; goto a; a: i++; b: i++; c: i++; i; }));
    ASSERT(2, ({ int i=0; goto e; d: i++; e: i++; f: i++; i; }));
    ASSERT(1, ({ int i=0; goto i; g: i++; h: i++; i: i++; i; }));

    ASSERT(3, ({ int i=0; for(;i<10;i++) { if (i == 3) break; } i; }));
    ASSERT(4, ({ int i=0; while (1) { if (i++ == 3) break; } i; }));
    ASSERT(3, ({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break; } i; }));
    ASSERT(4, ({ int i=0; while (1) { while(1) break; if (i++ == 3) break; } i; }));

    ASSERT(10, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } i; }));
    ASSERT(6, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } j; }));
    ASSERT(10, ({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } i; }));
    ASSERT(5, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break; } j; }));

    ASSERT(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j; }));
    ASSERT(3, ({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i; }));

    ASSERT(3, ret3());
    ASSERT(5, ret5());
    ASSERT(8, add(3, 5));
    ASSERT(2, sub(5, 3));
    ASSERT(21, add6(1, 2, 3, 4, 5, 6));
    ASSERT(66, add6(1, 2, add6(3, 4, 5, 6, 7, 8), 9, 10, 11));
    ASSERT(136, add6(1, 2, add6(3, add6(4, 5, 6, 7, 8, 9), 10, 11, 12, 13), 14, 15, 16));
    ASSERT(55, fib(9));

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

    ASSERT(1, sizeof(char));
    ASSERT(2, sizeof(short));
    ASSERT(4, sizeof(int));
    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(char *));
    ASSERT(8, sizeof(int *));
    ASSERT(8, sizeof(long *));
    ASSERT(8, sizeof(int **));
    ASSERT(8, sizeof(int(*)[4]));
    ASSERT(32, sizeof(int*[4]));
    ASSERT(16, sizeof(int[4]));
    ASSERT(48, sizeof(int[3][4]));
    ASSERT(8, sizeof(struct {int a; int b;}));

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

    ASSERT(1, ({ int x[3]={1,2,3}; x[0]; }));
    ASSERT(2, ({ int x[3]={1,2,3}; x[1]; }));
    ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));

    ASSERT(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }));
    ASSERT(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }));
    ASSERT(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }));

    ASSERT(0, ({ int x[3]={}; x[0]; }));
    ASSERT(0, ({ int x[3]={}; x[1]; }));
    ASSERT(0, ({ int x[3]={}; x[2]; }));

    ASSERT(2, ({ int x[2][3]={{1,2}}; x[0][1]; }));
    ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][0]; }));
    ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][2]; }));

    ASSERT(4, ({ int x[]={1,2,3,4}; x[3]; }));
    ASSERT(16, ({ int x[]={1,2,3,4}; sizeof(x); }));
    ASSERT(5, ({ int x[][2] = {{1, 2}, {3, 4}, {5, 6}}; x[2][0]; }));
    ASSERT(24, ({ int x[][2] = {{1, 2}, {3, 4}, {5, 6}}; sizeof(x); }));

    ASSERT(0, g1);
    ASSERT(3, ({ g1=3; g1; }));
    ASSERT(7, ({ g1=3; g2=4; g1+g2; }));
    ASSERT(0, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[0]; }));
    ASSERT(1, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[1]; }));
    ASSERT(2, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[2]; }));
    ASSERT(3, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[3]; }));

    ASSERT(1, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; }));
    ASSERT(2, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; }));
    ASSERT(1, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; }));
    ASSERT(2, ({ struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3; x.b; }));
    ASSERT(3, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; }));

    ASSERT(0, ({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; }));
    ASSERT(1, ({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; }));
    ASSERT(2, ({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; }));
    ASSERT(3, ({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; }));

    ASSERT(6, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; }));
    ASSERT(7, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7; p[3]; }));

    ASSERT(6, ({ struct { struct { char b; } a; } x; x.a.b=6; x.a.b; }));

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

    ASSERT(3, ({ struct t {char a;} x; struct t *y = &x; x.a=3; y->a; }));
    ASSERT(3, ({ struct t {char a;} x; struct t *y = &x; y->a=3; x.a; }));

    ASSERT(3, ({ struct t {int a; int b;}; struct t x; x.a=3; struct t y=x; y.a; }));
    ASSERT(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x; y.a; }));
    ASSERT(7, ({ struct t {int a; int b;}; struct t x; x.a=7; struct t y; struct t *p=&x; struct t *q=&y; *q=*p; y.a; }));
    ASSERT(5, ({ struct t {char a; char b;} x; struct t y; x.a=5; y=x; y.a; }));
    ASSERT(1, ({ struct t {int a; struct {int x; struct {char p; char q;} y;} b;}; struct t x={1,{2,{3,4}}}; struct t y=x; y.a; }));
    ASSERT(2, ({ struct t {int a; struct {int x; struct {char p; char q;} y;} b;}; struct t x={1,{2,{3,4}}}; struct t y=x; y.b.x; }));
    ASSERT(3, ({ struct t {int a; struct {int x; struct {char p; char q;} y;} b;}; struct t x={1,{2,{3,4}}}; struct t y=x; y.b.y.p; }));
    ASSERT(4, ({ struct t {int a; struct {int x; struct {char p; char q;} y;} b;}; struct t x={1,{2,{3,4}}}; struct t y=x; y.b.y.q; }));
    ASSERT(1, ({ struct t {int a; struct {char p; char q;} b[3];}; struct t x={1,{{2,3}, {4},{5}}}; struct t y=x; y.a; }));
    ASSERT(2, ({ struct t {int a; struct {char p; char q;} b[3];}; struct t x={1,{{2,3}, {4},{5}}}; struct t y=x; y.b[0].p; }));
    ASSERT(3, ({ struct t {int a; struct {char p; char q;} b[3];}; struct t x={1,{{2,3}, {4},{5}}}; struct t y=x; y.b[0].q; }));
    ASSERT(4, ({ struct t {int a; struct {char p; char q;} b[3];}; struct t x={1,{{2,3}, {4},{5}}}; struct t y=x; y.b[1].p; }));
    ASSERT(0, ({ struct t {int a; struct {char p; char q;} b[3];}; struct t x={1,{{2,3}, {4},{5}}}; struct t y=x; y.b[1].q; }));

    ASSERT(1, ({ struct {int a; int b; int c;} x={1,2,3}; x.a; }));
    ASSERT(2, ({ struct {int a; int b; int c;} x={1,2,3}; x.b; }));
    ASSERT(3, ({ struct {int a; int b; int c;} x={1,2,3}; x.c; }));
    ASSERT(1, ({ struct {int a; int b; int c;} x={1}; x.a; }));
    ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.b; }));
    ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.c; }));

    ASSERT(1, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a; }));
    ASSERT(2, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b; }));
    ASSERT(3, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a; }));
    ASSERT(4, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b; }));

    ASSERT(0, ({ struct {int a; int b;} x[2]={{1,2}}; x[1].b; }));

    ASSERT(0, ({ struct {int a; int b;} x={}; x.a; }));
    ASSERT(0, ({ struct {int a; int b;} x={}; x.b; }));

    ASSERT(8, ({ union { int a; char b[6]; } x; sizeof(x); }));
    ASSERT(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }));
    ASSERT(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }));
    ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }));
    ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }));

    ASSERT(42, g4);
    ASSERT(41, g5);
    ASSERT(47, g6);
    ASSERT(15, g7);
    ASSERT(4, g8);
    ASSERT(10, g9);
    ASSERT(10, g10);
    ASSERT(0, g11);
    ASSERT(1, g12);
    ASSERT(1, g13);
    ASSERT(1, g14);
    ASSERT(0, g15[0]);
    ASSERT(1, g15[1]);
    ASSERT(2, g15[2]);
    ASSERT(2, g16[0][1]);
    ASSERT(4, g16[1][0]);
    ASSERT(6, g16[1][2]);
    ASSERT(0, g17[0]);
    ASSERT(0, g17[1]);
    ASSERT(0, g17[2]);
    ASSERT(2, g18[0][1]);
    ASSERT(0, g18[1][0]);
    ASSERT(0, g18[1][2]);
    ASSERT(3, g19[3]);
    ASSERT(4, g20[2][0]);
    ASSERT(1, g21[0].a);
    ASSERT(2, g21[0].b);
    ASSERT(3, g21[1].a);
    ASSERT(4, g21[1].b);
    ASSERT(1, g22[0].a[0]);
    ASSERT(2, g22[0].a[1]);
    ASSERT(0, g22[1].a[0]);
    ASSERT(0, g22[1].a[1]);

    ASSERT(4, sizeof(g1));
    ASSERT(16, sizeof(g3));

    ASSERT(1, ({ char x=1; x; }));
    ASSERT(1, ({ char x=1; char y=2; x; }));
    ASSERT(2, ({ char x=1; char y=2; y; }));

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(10, ({ char x[10]; sizeof(x); }));
    ASSERT(1, ({ sub_char(7, 3, 3); }));
    ASSERT(1, sub_short(7, 3, 3));
    ASSERT(1, sub_long(7, 3, 3));

    ASSERT(0, ""[0]);
    ASSERT(1, sizeof(""));

    ASSERT(97, "abc"[0]);
    ASSERT(98, "abc"[1]);
    ASSERT(99, "abc"[2]);
    ASSERT(0, "abc"[3]);
    ASSERT(4, sizeof("abc"));

    ASSERT(7, "\a"[0]);
    ASSERT(8, "\b"[0]);
    ASSERT(9, "\t"[0]);
    ASSERT(10, "\n"[0]);
    ASSERT(11, "\v"[0]);
    ASSERT(12, "\f"[0]);
    ASSERT(13, "\r"[0]);
    ASSERT(27, "\e"[0]);
    ASSERT(34, "\""[0]);
    ASSERT(92, "\\"[0]);
    ASSERT(0, "\0"[0]);

    ASSERT(106, "\j"[0]);
    ASSERT(107, "\k"[0]);
    ASSERT(108, "\l"[0]);

    ASSERT(7, "\ax\ny"[0]);
    ASSERT(120, "\ax\ny"[1]);
    ASSERT(10, "\ax\ny"[2]);
    ASSERT(121, "\ax\ny"[3]);

    ASSERT(2, ({ int x=2; { int x=3; } x; }));
    ASSERT(2, ({ int x=2; { int x=3; } int y=4; x; }));
    ASSERT(3, ({ int x=2; { x=3; } x; }));

    ASSERT(3, (1,2,3));
    ASSERT(3, ({ int i=2; int j=3; int k=(i, j); k; }));
    ASSERT(2, ({ int i=2; int j=3; int k=(j, i); k; }));

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

    ASSERT(131585, (int)8590066177);
    ASSERT(513, (short)8590066177);
    ASSERT(1, (char)8590066177);
    ASSERT(1, (long)1);
    ASSERT(0, (long)&*(int *)0);
    ASSERT(15, ({ short x=32527; char y=(char)x; y;}));
    ASSERT(3855, ({ int x=2147421967; short y=(short)x; y;}));
    ASSERT(513, ({ int x=512; *(char *)&x=1; x; }));
    ASSERT(5, ({ int x=5; long y=(long)&x; *(int*)y; }));

    (void)1;

    printf("OK\n");

    return 0;
}
