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

int ret3() {return 3;}
int ret5() {return 5;}
int add(int x, int y) {return x+y;}
int sub(int x, int y) {return x-y;}
int add6(int a, int b, int c, int d, int e, int f) {return a+b+c+d+e+f;}
int sub_char(char a, char b, char c) {return a-b-c;}
int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }

int main() {
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

    ASSERT(3, ({ int a; a=3; a; }));
    ASSERT(8, ({ int a=3; int z=5; a+z; }));
    ASSERT(6, ({ int a; int b; a=b=3; a+b; }));
    ASSERT(3, ({ int foo; foo=3; foo; }));
    ASSERT(8, ({ int foo123=3; int bar=5; foo123+bar; }));

    ASSERT(3, ({ 1; {2;} 3; }));
    ASSERT(5, ({ ;;; 5; }));

    ASSERT(3, ({ int x; if (0) x=2; else x=3; x; }));
    ASSERT(3, ({ int x; if (1-1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if (2-1) x=2; else x=3; x; }));
    ASSERT(4, ({ int x; if (0) { x=1; x=2; x=3; } else { x=4; } x; }));
    ASSERT(3, ({ int x; if (1) { x=1; x=2; x=3; } else { x=4; } x; }));

    ASSERT(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));

    ASSERT(10, ({ int i=0; while(i<10) { i=i+1; } i; }));

    ASSERT(3, ret3());
    ASSERT(5, ret5());
    ASSERT(8, add(3, 5));
    ASSERT(2, sub(5, 3));
    ASSERT(21, add6(1,2,3,4,5,6));
    ASSERT(66, add6(1,2,add6(3,4,5,6,7,8),9,10,11));
    ASSERT(136, add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16));
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

    ASSERT(8, ({ int x; sizeof(x); }));
    ASSERT(8, ({ int x; sizeof x; }));
    ASSERT(8, ({ int *x; sizeof(x); }));
    ASSERT(32, ({ int x[4]; sizeof(x); }));
    ASSERT(8, ({ int x=1; sizeof(x=2); }));
    ASSERT(1, ({ int x=1; sizeof(x=2); x; }));

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

    ASSERT(0, g1);
    ASSERT(3, ({ g1=3; g1; }));
    ASSERT(7, ({ g1=3; g2=4; g1+g2; }));
    ASSERT(0, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[0]; }));
    ASSERT(1, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[1]; }));
    ASSERT(2, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[2]; }));
    ASSERT(3, ({ g3[0]=0; g3[1]=1; g3[2]=2; g3[3]=3; g3[3]; }));

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

    ASSERT(8, sizeof(g1));
    ASSERT(32, sizeof(g3));

    ASSERT(1, ({ char x=1; x; }));
    ASSERT(1, ({ char x=1; char y=2; x; }));
    ASSERT(2, ({ char x=1; char y=2; y; }));

    ASSERT(1, ({ char x; sizeof(x); }));
    ASSERT(10, ({ char x[10]; sizeof(x); }));
    ASSERT(1, ({ sub_char(7, 3, 3); }));

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

    ASSERT(2,({ int x=2; { int x=3; } x; }));
    ASSERT(2,({ int x=2; { int x=3; } int y=4; x; }));
    ASSERT(3,({ int x=2; { x=3; } x; }));

    printf("OK\n");

    return 0;
}
