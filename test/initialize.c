#include "test.h"

int g15[3]={0,1,2};
int g16[2][3]={{1,2,3},{4,5,6}};
int g17[3]={};
int g18[2][3]={{1,2}};
int g19[]={0,1,2,3};
int g20[][2]={{0,1},{2,3},{4,5}};
struct {char a; int b;} g21[2]={{1,2},{3,4}};
struct {int a[2];} g22[2]={{{1,2}}};
union {int a; char b;} g23[2]={{515},{0}};
union {int a; char b[9];} g24={515};
char g25[4]="abc";
char g26[]="abc";
char g27[2][4]={"abc","def"};
char g28[][4]={"abc","def"};
enum {five=5, six, seven} g29=five;
char *g30="xyz";
char *g31[]={"foo","bar"};

int main() {
    int l3[3]={0,1,2};
    int l4[]={0,1,2,3};
    int l5[][2]={{0,1},{2,3},{4,5}};

    ASSERT(0, l3[0]);
    ASSERT(1, l3[1]);
    ASSERT(2, l3[2]);
    ASSERT(3, l4[3]);
    ASSERT(4, l5[2][0]);

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
    ASSERT(515, g23[0].a);
    ASSERT(3, g23[0].b);
    ASSERT(0, g23[1].a);
    ASSERT(0, g23[1].b);
    ASSERT(515, g24.a);
    ASSERT(3, g24.b[0]);
    ASSERT(2, g24.b[1]);
    ASSERT(0, g24.b[2]);
    ASSERT(0, g24.b[3]);
    ASSERT(97, g25[0]);
    ASSERT(98, g25[1]);
    ASSERT(99, g26[2]);
    ASSERT(0, g26[3]);
    ASSERT(97, g27[0][0]);
    ASSERT(100, g27[1][0]);
    ASSERT(99, g28[0][2]);
    ASSERT(0, g28[1][3]);
    ASSERT(5, g29);
    ASSERT(0, strcmp(g30,"xyz"));
    ASSERT(0, strcmp(g31[0],"foo"));
    ASSERT(0, strcmp(g31[1],"bar"));

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
    ASSERT(5, ({ int x[][2]={{1,2},{3,4},{5,6}}; x[2][0]; }));
    ASSERT(24, ({ int x[][2]={{1,2},{3,4},{5, 6}}; sizeof(x); }));

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

    ASSERT(3, ({ union t {int a; char b[9];} x={515}; x.b[0]; }));
    ASSERT(2, ({ union t {int a; char b[9];} x={515}; x.b[1]; }));
    ASSERT(0, ({ union t {int a; char b[9];} x={515}; x.b[2]; }));
    ASSERT(0, ({ union t {int a; char b[9];} x={515}; x.b[3]; }));

    ASSERT(16909060, ({union {struct {char a; char b; char c; char d;} e; int f;} x={{4,3,2,1}}; x.f; }));

    ASSERT(97, ({ char x[4]="abc"; x[0]; }));
    ASSERT(99, ({ char x[4]="abc"; x[2]; }));
    ASSERT(98, ({ char x[]="abc"; x[1]; }));
    ASSERT(0, ({ char x[]="abc"; x[3]; }));
    ASSERT(97, ({ char x[2][4]={"abc","def"}; x[0][0]; }));
    ASSERT(0, ({ char x[2][4]={"abc","def"}; x[0][3]; }));
    ASSERT(100, ({ char x[][4]={"abc","def"}; x[1][0]; }));
    ASSERT(102, ({ char x[][4]={"abc","def"}; x[1][2]; }));

    ASSERT(0, ({ char *x="xyz"; strcmp(x,"xyz"); }));
    ASSERT(0, ({ char *x[]={"foo","bar"}; strcmp(x[0],"foo"); }));
    ASSERT(0, ({ char *x[]={"foo","bar"}; strcmp(x[1],"bar"); }));

    ASSERT(0, ({ enum { zero, one, two } x=zero; x; }));
    ASSERT(1, ({ enum { zero, one, two } x=one; x; }));
    ASSERT(2, ({ enum { zero, one, two } x=two; x; }));

    return 0;
}
