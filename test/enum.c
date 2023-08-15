#include "test.h"

enum gE;
typedef enum gE gE;

gE extE;

enum gE { A, B, };

int main() {
    enum E;
    typedef enum E E;
    enum E { C, D };

    ASSERT(0, ({ enum {zero, one, two}; zero; }));
    ASSERT(1, ({ enum {zero, one, two}; one; }));
    ASSERT(2, ({ enum {zero, one, two}; two; }));
    ASSERT(5, ({ enum {five=5, six, seven}; five; }));
    ASSERT(6, ({ enum {five=5, six, seven}; six; }));
    ASSERT(0, ({ enum {zero, five=5, three=3, four}; zero; }));
    ASSERT(5, ({ enum {zero, five=5, three=3, four}; five; }));
    ASSERT(3, ({ enum {zero, five=5, three=3, four}; three; }));
    ASSERT(4, ({ enum {zero, five=5, three=3, four}; four; }));
    ASSERT(4, ({ enum {zero, one, two } x; sizeof(x); }));
    ASSERT(4, ({ enum t { zero, one, two }; enum t y; sizeof(y); }));

    ASSERT(0, ({ enum {zero=(0), one=(1&1), two=(1<<1)}; zero; }));
    ASSERT(1, ({ enum {zero=(0), one=(1&1), two=(1<<1)}; one; }));
    ASSERT(2, ({ enum {zero=(0), one=(1&1), two=(1<<1)}; two; }));

    return 0;
}
