#include "9cc.h"

bool is_type_of(TypeKind kind, Type *ty) { return ty->kind == kind; }

bool is_integer(Type *ty) {
    return is_type_of(TY_INT, ty) || is_type_of(TY_CHAR, ty);
}

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

    for (Node *n = node->args; n; n = n->next) {
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
        if (is_type_of(TY_ARY, node->lhs->ty)) {
            error_tok(node->lhs->tok, "左辺値ではありません");
        }
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_LE:
    case ND_LT:
    case ND_NE:
    case ND_NOT:
    case ND_LOGAND:
    case ND_LOGOR:
    case ND_NUM:
    case ND_FUNCALL:
        node->ty = ty_int;
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_ADDR:
        if (is_type_of(TY_ARY, node->lhs->ty)) {
            node->ty = pointer_to(node->lhs->ty->base);
        } else {
            node->ty = pointer_to(node->lhs->ty);
        }
        return;
    case ND_DEREF:
        if (!node->lhs->ty->base) {
            error_tok(node->tok, "参照外しできません");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_COMMA:
        node->ty = node->rhs->ty;
        return;
    case ND_STMT_EXPR:
        if (node->body) {
            Node *stmt = node->body;
            for (; stmt->next; stmt = stmt->next) {}
            if (stmt->kind == ND_EXPR_STMT) {
                node->ty = stmt->lhs->ty;
                return;
            }
        }
        error_tok(node->tok, "voidを返すことはできません");
        return;
    default:;
    }
}
