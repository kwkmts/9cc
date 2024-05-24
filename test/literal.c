#include "test.h"

typedef struct Tree {
    int val;
    struct Tree *lhs;
    struct Tree *rhs;
} Tree;

Tree *tree=&(Tree){
    1,
    &(Tree){
        2,
        &(Tree){3,0,0},
        &(Tree){4,0,0}
    },
    0
};

Tree tree2=(Tree){1,0,0};

int main() {
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

    ASSERT(97, 'a');
    ASSERT(10, '\n');

    ASSERT('a', "abc"[0]);
    ASSERT('\0', "abc"[3]);

    ASSERT(511, 0777);
    ASSERT(0, 0x0);
    ASSERT(10, 0xa);
    ASSERT(10, 0XA);
    ASSERT(48879, 0xbeef);
    ASSERT(48879, 0xBEEF);
    ASSERT(48879, 0XBEEF);

    ASSERT(1, (int){1});
    ASSERT(2, ((int[]){0,1,2})[2]);
    ASSERT('a', ((struct {char a; int b;}){'a',3}).a);
    ASSERT(3, ({ int x=3; (int){x}; }));
    (int){3}=5;

    ASSERT(1, tree->val);
    ASSERT(2, tree->lhs->val);
    ASSERT(3, tree->lhs->lhs->val);
    ASSERT(4, tree->lhs->rhs->val);
    ASSERT(1, tree2.val);

    ASSERT(4, sizeof(0));
    ASSERT(8, sizeof(0L));
    ASSERT(8, sizeof(0LU));
    ASSERT(8, sizeof(0UL));
    ASSERT(8, sizeof(0LL));
    ASSERT(8, sizeof(0LLU));
    ASSERT(8, sizeof(0Ull));
    ASSERT(8, sizeof(0l));
    ASSERT(8, sizeof(0ll));
    ASSERT(8, sizeof(0x0L));
    ASSERT(4, sizeof(2147483647));
    ASSERT(8, sizeof(2147483648));
    ASSERT(-1, 0xffffffffffffffff);
    ASSERT(8, sizeof(0xffffffffffffffff));
    ASSERT(4, sizeof(4294967295U));
    ASSERT(8, sizeof(4294967296U));

    ASSERT(3, -1U>>30);
    ASSERT(3, -1Ul>>62);
    ASSERT(3, -1ull>>62);

    ASSERT(1, 0xffffffffffffffffl>>63);
    ASSERT(1, 0xffffffffffffffffll>>63);

    ASSERT(-1, 18446744073709551615);
    ASSERT(8, sizeof(18446744073709551615));
    ASSERT(-1, 18446744073709551615>>63);

    ASSERT(-1, 0xffffffffffffffff);
    ASSERT(8, sizeof(0xffffffffffffffff));
    ASSERT(1, 0xffffffffffffffff>>63);

    ASSERT(-1, 01777777777777777777777);
    ASSERT(8, sizeof(01777777777777777777777));
    ASSERT(1, 01777777777777777777777>>63);

    ASSERT(8, sizeof(2147483648));
    ASSERT(4, sizeof(2147483647));

    ASSERT(8, sizeof(0x1ffffffff));
    ASSERT(4, sizeof(0xffffffff));
    ASSERT(1, 0xffffffff>>31);

    ASSERT(8, sizeof(040000000000));
    ASSERT(4, sizeof(037777777777));
    ASSERT(1, 037777777777>>31);

    ASSERT(-1, 1<<31>>31);
    ASSERT(-1, 01<<31>>31);
    ASSERT(-1, 0x1<<31>>31);

    ASSERT(4, sizeof(8.f));
    ASSERT(4, sizeof(0.3F));
    ASSERT(8, sizeof(0.));
    ASSERT(8, sizeof(.0));
    ASSERT(8, sizeof(5.l));
    ASSERT(8, sizeof(2.0L));

    ASSERT(300000000, (int)3e+8);
    ASSERT(16, (int)0x10.1p0);
    ASSERT(1000, (int).1E4f);

    return 0;
}
