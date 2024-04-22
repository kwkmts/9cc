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
    ASSERT(1, sizeof(signed char));
    ASSERT(2, sizeof(short));
    ASSERT(2, sizeof(int short));
    ASSERT(2, sizeof(short int));
    ASSERT(2, sizeof(signed short));
    ASSERT(2, sizeof(int short signed));
    ASSERT(4, sizeof(int));
    ASSERT(4, sizeof(signed int));
    ASSERT(4, sizeof(signed));
    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(signed long));
    ASSERT(8, sizeof(signed long int));
    ASSERT(8, sizeof(long long));
    ASSERT(8, sizeof(signed long long));
    ASSERT(8, sizeof(signed long long int));
    ASSERT(8, sizeof(char *));
    ASSERT(8, sizeof(int *));
    ASSERT(8, sizeof(long *));
    ASSERT(8, sizeof(int **));
    ASSERT(8, sizeof(int(*)[4]));
    ASSERT(32, sizeof(int*[4]));
    ASSERT(16, sizeof(int[4]));
    ASSERT(48, sizeof(int[3][4]));
    ASSERT(8, sizeof(struct {int a; int b;}));

    ASSERT(1, _Alignof(char));
    ASSERT(2, _Alignof(short));
    ASSERT(4, _Alignof(int));
    ASSERT(8, _Alignof(long));
    ASSERT(8, _Alignof(long long));
    ASSERT(1, _Alignof(char[3]));
    ASSERT(4, _Alignof(int[3]));
    ASSERT(1, _Alignof(struct {char a; char b;}[2]));
    ASSERT(8, _Alignof(struct {char a; long b;}[2]));

    ASSERT(1, ({ _Alignas(char) char x, y; &x-&y; }));
    ASSERT(8, ({ _Alignas(long) char x, y; &x-&y; }));
    ASSERT(32, ({ _Alignas(32) char x, y; &x-&y; }));
    ASSERT(32, ({ _Alignas(32) int *x, *y; ((char *)&x)-((char *)&y); }));
    ASSERT(16, ({ struct { _Alignas(16) char x, y; } a; &a.y-&a.x; }));
    ASSERT(8, ({ struct T { _Alignas(8) char a; }; _Alignof(struct T); }));

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
    ASSERT(1, ({ unsigned x=4294967295; (long)x==4294967295; }));

    ASSERT(1, sizeof((char)1));
    ASSERT(2, sizeof((short)1));
    ASSERT(4, sizeof((int)1));
    ASSERT(8, sizeof((long)1));

    ASSERT(4, sizeof(float));
    ASSERT(8, sizeof(double));

    ASSERT(3, (char)3.0);
    ASSERT(1000, (short)1000.3);
    ASSERT(3, (int)3.99);
    ASSERT(3, (float)3.5);
    ASSERT(5, (double)(float)5.5);
    ASSERT(3, (float)3);
    ASSERT(3, (double)3);
    ASSERT(3, (float)3L);
    ASSERT(3, (double)3L);

    ASSERT(35, (float)(char)35);
    ASSERT(35, (float)(short)35);
    ASSERT(35, (float)(int)35);
    ASSERT(35, (float)(long)35);
    ASSERT(35, (float)(unsigned char)35);
    ASSERT(35, (float)(unsigned short)35);
    ASSERT(35, (float)(unsigned int)35);
    ASSERT(35, (float)(unsigned long)35);

    ASSERT(35, (double)(char)35);
    ASSERT(35, (double)(short)35);
    ASSERT(35, (double)(int)35);
    ASSERT(35, (double)(long)35);
    ASSERT(35, (double)(unsigned char)35);
    ASSERT(35, (double)(unsigned short)35);
    ASSERT(35, (double)(unsigned int)35);
    ASSERT(35, (double)(unsigned long)35);

    ASSERT(35, (char)(float)35);
    ASSERT(35, (short)(float)35);
    ASSERT(35, (int)(float)35);
    ASSERT(35, (long)(float)35);
    ASSERT(35, (unsigned char)(float)35);
    ASSERT(35, (unsigned short)(float)35);
    ASSERT(35, (unsigned int)(float)35);
    ASSERT(35, (unsigned long)(float)35);

    ASSERT(35, (char)(double)35);
    ASSERT(35, (short)(double)35);
    ASSERT(35, (int)(double)35);
    ASSERT(35, (long)(double)35);
    ASSERT(35, (unsigned char)(double)35);
    ASSERT(35, (unsigned short)(double)35);
    ASSERT(35, (unsigned int)(double)35);
    ASSERT(35, (unsigned long)(double)35);

    ASSERT(-2147483648, (double)(unsigned long)(long)-1);

    ASSERT(1, 0.1==0.1);
    ASSERT(0, 0.1==0.2);
    ASSERT(1, 2.0==2);
    ASSERT(0, 5.1<5);
    ASSERT(0, 5.0<5);
    ASSERT(1, 4.9<5);
    ASSERT(0, 5.1<=5);
    ASSERT(1, 5.0<=5);
    ASSERT(1, 4.9<=5);

    ASSERT(1, 0.1f==0.1f);
    ASSERT(0, 0.1f==0.2f);
    ASSERT(1, 2.0f==2);
    ASSERT(0, 5.1f<5);
    ASSERT(0, 5.0f<5);
    ASSERT(1, 4.9f<5);
    ASSERT(0, 5.1f<=5);
    ASSERT(1, 5.0f<=5);
    ASSERT(1, 4.9f<=5);

    ASSERT(6, 2.3+3.8);
    ASSERT(-1, 2.3-3.8);
    ASSERT(-3, -3.8);
    ASSERT(13, 3.3*4);
    ASSERT(2, 5.0/2);

    ASSERT(6, 2.3f+3.8f);
    ASSERT(6, 2.3f+3.8);
    ASSERT(-1, 2.3f-3.8);
    ASSERT(-3, -3.8f);
    ASSERT(13, 3.3f*4);
    ASSERT(2, 5.0f/2);

    ASSERT(0, 0.0/0.0 == 0.0/0.0);
    ASSERT(1, 0.0/0.0 != 0.0/0.0);

    ASSERT(0, 0.0/0.0 < 0);
    ASSERT(0, 0.0/0.0 <= 0);
    ASSERT(0, 0.0/0.0 > 0);
    ASSERT(0, 0.0/0.0 >= 0);

    ASSERT(4, sizeof(1.f+2));
    ASSERT(8, sizeof(1.0+2));
    ASSERT(4, sizeof(1.f-2));
    ASSERT(8, sizeof(1.0-2));
    ASSERT(4, sizeof(1.f*2));
    ASSERT(8, sizeof(1.0*2));
    ASSERT(4, sizeof(1.f/2));
    ASSERT(8, sizeof(1.0/2));

    ASSERT(0, 0.0&&0.0);
    ASSERT(0, 0.0&&0.1);
    ASSERT(0, 0.3&&0.0);
    ASSERT(1, 0.3&&0.5);
    ASSERT(0, 0.0||0.0);
    ASSERT(1, 0.0||0.1);
    ASSERT(1, 0.3||0.0);
    ASSERT(1, 0.3||0.5);
    ASSERT(0, !3.);
    ASSERT(1, !0.);
    ASSERT(0, !3.f);
    ASSERT(1, !0.f);

    ASSERT(5, 0.0 ? 3 : 5);
    ASSERT(3, 1.2 ? 3 : 5);


    assert(1, size\
of(char), \
           "sizeof(char)");

    return 0;
}
