#include "test.h"

int ret3(){ return 3; }
int ret5(){ return 5; }
int ret42();
int ret_n(int);
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
int g1;
int *g1_ptr() { return &g1; }
char int_to_char(int x) { return x; }
int div_long(long a, long b) {
    return a / b;
}
_Bool bool_fn_add(_Bool x) { return x + 1; }
_Bool bool_fn_sub(_Bool x) { return x - 1; }
int (*ret_fnptr(int (*fn)(int,int)))(int,int) { return fn; }
int param_decay(int x()) { return x(); }
int param_decay2(int x[]) { return x[0]; }
int add_all(int n, ...);
void fmt(char *buf, char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);

    __builtin_va_list ap2;
    __builtin_va_copy(ap2, ap);
    vsprintf(buf, fmt, ap2);
    __builtin_va_end(ap);
    __builtin_va_end(ap2);
}
int sum1(int x, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, x);

    for (;;) {
        int y = __builtin_va_arg(ap, int);
        if (y == 0)
            return x;
        x += y;
    }
}
void a(){}
void b(int a){}
const char *func_fn() { return __func__; }
void ret_none() { return; }

int main() {
    ASSERT(3, ret3());
    ASSERT(5, ret5());
    ASSERT(42, ret42());
    ASSERT(3, ret_n(3));
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

    g1 = 3;

    ASSERT(3, *g1_ptr());
    ASSERT(5, int_to_char(261));
    ASSERT(-5, div_long(-10, 2));

    ASSERT(1, bool_fn_add(3));
    ASSERT(0, bool_fn_sub(3));
    ASSERT(1, bool_fn_add(-3));
    ASSERT(0, bool_fn_sub(-3));
    ASSERT(1, bool_fn_add(0));
    ASSERT(1, bool_fn_sub(0));

    ASSERT(5, (add)(2,3));
    ASSERT(5, (&add)(2,3));
    ASSERT(7, ({ int (*fn)(int,int)=add; fn(2,5); }));
    ASSERT(7, ({ int (*fn)(int,int)=add; (*fn)(2,5); }));
    ASSERT(7, ({ int (*fn)(int,int)=add; (&**&fn)(2,5); }));
    ASSERT(7, ret_fnptr(add)(2,5));

    ASSERT(3, param_decay(ret3));
    ASSERT(3, ({ int x[2]; x[0]=3; param_decay2(x); }));

    ASSERT(6, add_all(3,1,2,3));
    ASSERT(5, add_all(4,1,2,3,-1));
    ASSERT(0, ({ char buf[100]; sprintf(buf,"%d %d %s",1,2,"foo"); strcmp("1 2 foo",buf); }));

    { char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); printf("%s\n", buf); }
    ASSERT(0, ({ char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }));

    ASSERT(6, sum1(1, 2, 3, 0));

    ASSERT(5, sizeof(__func__));
    ASSERT(0, strcmp("main", __func__));
    ASSERT(0, strcmp("func_fn", func_fn()));

    ret_none();

    return 0;
}

int ret42(){ return 42; }
int ret_n(int n){ return n; }
