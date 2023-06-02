#include "test.h"

int main() {
    ASSERT((long)-5, -10 + (long)5);
    ASSERT((long)-15, -10 - (long)5);
    ASSERT((long)-50, -10 * (long)5);
    ASSERT((long)-2, -10 / (long)5);

    ASSERT(1, -2 < (long)-1);
    ASSERT(1, -2 <= (long)-1);
    ASSERT(0, -2 > (long)-1);
    ASSERT(0, -2 >= (long)-1);

    ASSERT(1, (long)-2 < -1);
    ASSERT(1, (long)-2 <= -1);
    ASSERT(0, (long)-2 > -1);
    ASSERT(0, (long)-2 >= -1);

    ASSERT(0, 2147483647 + 2147483647 + 2);
    ASSERT((long)-1, ({ long x; x=-1; x; }));

    ASSERT(1, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0]; }));
    ASSERT(0, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1]; }));
    ASSERT(5, ({ typedef struct {char a;} T; T x; T y; x.a=5; y=x; y.a; }));

    ASSERT(0, -1<(unsigned)1);
    ASSERT(254, (char)127+(char)127);
    ASSERT(65534, (short)32767+(short)32767);
    ASSERT(2147483647, ((unsigned)-1)>>1);
    ASSERT(2147483598, ((unsigned)-100)/2);
    ASSERT(9223372036854775758, ((unsigned long)-100)/2);
    ASSERT(0, ((long)-1)/(unsigned)100);
    ASSERT(2, ((unsigned)-100)%7);
    ASSERT(6, ((unsigned long)-100)%9);

    ASSERT(4, sizeof((char)1+(char)1));
    ASSERT(4, sizeof((short)1+(short)1));
    ASSERT(4, sizeof(1?2:3));
    ASSERT(4, sizeof(1?(short)2:(char)3));
    ASSERT(8, sizeof(1?(long)2:(char)3));

    return 0;
}
