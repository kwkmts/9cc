#include "test.h"

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

int main() {
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

    ASSERT(0, 0);
    ASSERT(42, 42);
    ASSERT(21, 5+20-4);
    ASSERT(41, 12 + 34 - 5);
    ASSERT(47, 5+6*7);
    ASSERT(15, 5*(9-6));
    ASSERT(4, (3+5)/2);
    ASSERT(10, -10+20);
    ASSERT(10, - -10);
    ASSERT(10, - -+10);
    ASSERT(5, 17%6);

    ASSERT(0, 0==1);
    ASSERT(1, 42==42);
    ASSERT(1, 0!=1);
    ASSERT(0, 42!=42);

    ASSERT(1, 0<1);
    ASSERT(0, 1<1);
    ASSERT(0, 2<1);
    ASSERT(1, 0<=1);
    ASSERT(1, 1<=1);
    ASSERT(0, 2<=1);

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

    1?2:(void)1;

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

    ASSERT(3, (1,2,3));
    ASSERT(3, ({ int i=2; int j=3; int k=(i,j); k; }));
    ASSERT(2, ({ int i=2; int j=3; int k=(j,i); k; }));

    ASSERT(1, sizeof(char));
    ASSERT(2, sizeof(short));
    ASSERT(4, sizeof(int));
    ASSERT(8, sizeof(long));
    ASSERT(1, sizeof(unsigned char));
    ASSERT(2, sizeof(unsigned short));
    ASSERT(2, sizeof(int short unsigned));
    ASSERT(4, sizeof(unsigned int));
    ASSERT(4, sizeof(unsigned));
    ASSERT(8, sizeof(unsigned long));
    ASSERT(8, sizeof(unsigned long int));
    ASSERT(8, sizeof(char *));
    ASSERT(8, sizeof(int *));
    ASSERT(8, sizeof(long *));
    ASSERT(8, sizeof(int **));
    ASSERT(8, sizeof(int(*)[4]));
    ASSERT(32, sizeof(int*[4]));
    ASSERT(16, sizeof(int[4]));
    ASSERT(48, sizeof(int[3][4]));
    ASSERT(8, sizeof(struct {int a; int b;}));

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

    ASSERT(-1, (char)255);
    ASSERT(255, (unsigned char)255);
    ASSERT(-1, (short)65535);
    ASSERT(65535, (unsigned short)65535);
    ASSERT(-1, (int)0xffffffff);
    ASSERT(0xffffffff, (unsigned)0xffffffff);

    ASSERT(1, -1<1);
    ASSERT(-1, -1>>1);
    ASSERT(-1, (unsigned long)-1);
    ASSERT(-50, (-100)/2);
    ASSERT(-2, (-100)%7);
    ASSERT(65535, (int)(unsigned short)65535);
    ASSERT(65535, ({ unsigned short x = 65535; x; }));
    ASSERT(65535, ({ unsigned short x = 65535; (int)x; }));

    ASSERT(1, sizeof((char)1));
    ASSERT(2, sizeof((short)1));
    ASSERT(4, sizeof((int)1));
    ASSERT(8, sizeof((long)1));

    return 0;
}
