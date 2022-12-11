#include "9cc.h"

bool type_of(TypeKind kind, Type *ty) { return ty->kind == kind; }

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    ty->size = 8;
    return ty;
}

Type *array_of(Type *base, size_t len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARY;
    ty->base = base;
    ty->size = base->size * len;
    ty->ary_len = len;
    return ty;
}

void add_type(Node *node) {
    if (!node || node->ty) {
        return;
    }

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->after);

    for (Node *n = node->body; n; n = n->next) {
        add_type(n);
    }

    switch (node->kind) {
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
            node->ty = node->lhs->ty;
            return;
        case ND_ASSIGN:
            if (type_of(TY_ARY, node->lhs->ty)) {
                error("左辺値ではありません");
            }
            node->ty = node->lhs->ty;
            return;
        case ND_EQ:
        case ND_LE:
        case ND_LT:
        case ND_NE:
        case ND_NUM:
        case ND_FUNCALL:
            node->ty = ty_int;
            return;
        case ND_LVAR:
            node->ty = node->lvar->ty;
            return;
        case ND_ADDR:
            if (type_of(TY_ARY, node->lhs->ty)) {
                node->ty = pointer_to(node->lhs->ty->base);
            } else {
                node->ty = pointer_to(node->lhs->ty);
            }
            return;
        case ND_DEREF:
            if (!node->lhs->ty->base) {
                error("参照外しできません");
            }
            node->ty = node->lhs->ty->base;
            return;
        default:;
    }
}
