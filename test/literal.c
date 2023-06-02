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

    return 0;
}
