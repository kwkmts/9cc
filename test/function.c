#include "test.h"

int ret3(){ return 3; }
int ret5(){ return 5; }
int add(int x, int y){ return x+y; }
int sub(int x, int y){ return x-y; }
int add6(int a, int b, int c, int d, int e, int f){ return a+b+c+d+e+f; }
int sub_char(char a, char b, char c){ return a-b-c; }
int sub_short(short a, short b, short c){ return a-b-c; }
int sub_long(long a, long b, long c){ return a-b-c; }
int fib(int x){
    if(x<=1) return 1;
    return fib(x-1)+fib(x-2);
}
int counter() {
    static int i;
    static int j = 1+1;
    return i++ + j++;
}
static int static_fn() { return 3; }

int main() {
    ASSERT(3, ret3());
    ASSERT(5, ret5());
    ASSERT(8, add(3,5));
    ASSERT(2, sub(5,3));
    ASSERT(21, add6(1,2,3,4,5,6));
    ASSERT(66, add6(1,2,add6(3,4,5,6,7,8),9,10,11));
    ASSERT(136, add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16));
    ASSERT(55, fib(9));
    ASSERT(1, ({ sub_char(7,3,3); }));
    ASSERT(1, sub_short(7,3,3));
    ASSERT(1, sub_long(7,3,3));

    ASSERT(2, counter());
    ASSERT(4, counter());
    ASSERT(6, counter());

    ASSERT(3, static_fn());

    return 0;
}
