#include "test.h"

int ret3(){ return 3; }
int ret5(){ return 5; }
int ret42(void);
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
int sum2(double x, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, x);

    for (;;) {
        double y = __builtin_va_arg(ap, double);
        if (y == 0)
            return x;
        x += y;
    }
}
int add10_int(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j);
float add10_float(float x1, float x2, float x3, float x4, float x5, float x6, float x7, float x8, float x9, float x10);
double add10_double(double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8, double x9, double x10);
int many_args(int a, int b, int c, int d, int e, int f, int g, int h) {
    return g / h;
}
double many_args2(double a, double b, double c, double d, double e, double f,
                  double g, double h, double i, double j) {
    return i / j;
}
int many_args3(int a, double b, int c, int d, double e, int f, double g, int h,
               double i, double j, double k, double l, double m, int n, int o,
               double p) {
    return o / p;
}
void a(){}
void b(int a){}
const char *func_fn() { return __func__; }
void ret_none() { return; }
int (*fii[])(int,int)={add,&sub,*add,**sub};
unsigned abs_val(int x) {
    if (x < 0) {
        return -x;
    }
    return x;
}
double add_double(double x, double y);
float add_float(float x, float y);
float add_float3(float x, float y, float z) {
  return x + y + z;
}
double add_double3(double x, double y, double z) {
  return x + y + z;
}
double add_selected(int a, int b, int c, int d, int e, int f, int g, int h,
                    int i, int j, ...) {
    int arr[] = {a, b, c, d, e, f, g, h, i, j};
    int sum = 0;
    __builtin_va_list ap;
    __builtin_va_start(ap, j);
    for (;;) {
        int y = __builtin_va_arg(ap, int);
        if (y < 0)
            return sum;
        if (y < 10)
            sum += arr[y];
    }
}

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
    ASSERT(7, ({ int (*fn)(int,int)=&add; fn(2,5); }));
    ASSERT(7, ({ int (*fn)(int,int)=*add; fn(2,5); }));
    ASSERT(7, ({ int (*fn)(int,int)=**add; fn(2,5); }));
    ASSERT(7, ret_fnptr(add)(2,5));
    ASSERT(3, fii[0](1,2));
    ASSERT(-1, fii[1](1,2));
    ASSERT(3, fii[2](1,2));
    ASSERT(-1, fii[3](1,2));

    ASSERT(3, param_decay(ret3));
    ASSERT(3, ({ int x[2]; x[0]=3; param_decay2(x); }));

    ASSERT(6, add_all(3,1,2,3));
    ASSERT(5, add_all(4,1,2,3,-1));
    ASSERT(0, ({ char buf[100]; sprintf(buf,"%d %d %s",1,2,"foo"); strcmp("1 2 foo",buf); }));

    { char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); printf("%s\n", buf); }
    ASSERT(0, ({ char buf[100]; fmt(buf, "%d %d %s", 1, 2, "foo"); strcmp("1 2 foo", buf); }));

    ASSERT(6, sum1(1, 2, 3, 0));
    ASSERT(55, sum1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0));
    ASSERT(6, sum2(1., 2., 3., 0.));
    ASSERT(55, sum2(1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 0.));

    ASSERT(5, sizeof(__func__));
    ASSERT(0, strcmp("main", __func__));
    ASSERT(0, strcmp("func_fn", func_fn()));

    ASSERT(0, ({
               char buf[200];
               sprintf(buf,
                       "%d %.1f %.1f %.1f %d %d %.1f %d %d %d %d %.1f %d %d %.1f %.1f %.1f %.1f %d",
                       1,1.0,1.0,1.0,1,1,1.0,1,1,1,1,1.0,1,1,1.0,1.0,1.0,1.0,1);
               printf("%s\n",buf);
               strcmp("1 1.0 1.0 1.0 1 1 1.0 1 1 1 1 1.0 1 1 1.0 1.0 1.0 1.0 1",buf);
           }));

    ASSERT(55, add10_int(1,2,3,4,5,6,7,8,9,10));

    ASSERT(4, many_args(1,2,3,4,5,6,40,10));
    ASSERT(4, many_args2(1,2,3,4,5,6,7,8,40,10));
    ASSERT(8, many_args3(1,2,3,4,5,6,7,8,9,10,11,12,13,14,80,10));

    ASSERT(42, abs_val(42));
    ASSERT(42, abs_val(-42));

    ASSERT(6, add_float(2.3,3.8));
    ASSERT(6, add_double(2.3,3.8));

    ASSERT(7, add_float3(2.5,2.5,2.5));
    ASSERT(7, add_double3(2.5,2.5,2.5));

    ASSERT(0, ({ char buf[100]; fmt(buf, "%.1f", (float)3.5); strcmp(buf, "3.5"); }));

    ASSERT(11, add_selected(1,2,3,4,5,6,7,8,9,10,0,9,-1));
    ASSERT(25, add_selected(1,2,3,4,5,6,7,8,9,10,0,2,4,6,8,-1));
}

int ret42(void){ return 42; }
int ret_n(int n){ return n; }
