#include "9cc.h"

Type *ty_void = &(Type){TY_VOID, 1, 1};
Type *ty_bool = &(Type){TY_BOOL, 1, 1};
Type *ty_char = &(Type){TY_CHAR, 1, 1};
Type *ty_short = &(Type){TY_SHORT, 2, 2};
Type *ty_int = &(Type){TY_INT, 4, 4};
Type *ty_long = &(Type){TY_LONG, 8, 8};
Type *ty_uchar = &(Type){TY_CHAR, 1, 1, true};
Type *ty_ushort = &(Type){TY_SHORT, 2, 2, true};
Type *ty_uint = &(Type){TY_INT, 4, 4, true};
Type *ty_ulong = &(Type){TY_LONG, 8, 8, true};

bool is_type_of(TypeKind kind, Type *ty) { return ty->kind == kind; }

bool is_integer(Type *ty) {
    return is_type_of(TY_INT, ty) || is_type_of(TY_CHAR, ty) ||
           is_type_of(TY_SHORT, ty) || is_type_of(TY_LONG, ty) ||
           is_type_of(TY_BOOL, ty) || is_type_of(TY_ENUM, ty);
}

Type *copy_type(Type *ty) {
    Type *ret = calloc(1, sizeof(Type));
    *ret = *ty;
    return ret;
}

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    ty->size = ty->align = 8;
    return ty;
}

Type *array_of(Type *base, int len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARY;
    ty->base = base;
    ty->size = base->size * len;
    ty->align = base->align;
    ty->ary_len = len;
    return ty;
}

Type *enum_type() {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ENUM;
    ty->size = 4;
    ty->align = 4;
    return ty;
}

Type *func_type(Type *ret, Type *params) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->ret = ret;
    ty->params = params;
    return ty;
}

void add_const(Type **ty) {
    if (is_type_of(TY_ARY, *ty)) {
        add_const(&(*ty)->base);
        return;
    }

    *ty = copy_type(*ty);
    (*ty)->is_const = true;
}

static Type *get_common_type(Type *ty1, Type *ty2) {
    if (ty1->base) {
        return pointer_to(ty1->base);
    }

    if (ty1->size < 4) {
        ty1 = ty_int;
    }
    if (ty2->size < 4) {
        ty2 = ty_int;
    }
    if (ty1->size != ty2->size) {
        return ty1->size < ty2->size ? ty2 : ty1;
    }
    if (ty1->is_unsigned != ty2->is_unsigned) {
        return ty1->is_unsigned ? ty1 : ty2;
    }
    return ty1;
}

static void usual_arith_conv(Node **lhs, Node **rhs) {
    Type *ty = get_common_type((*lhs)->ty, (*rhs)->ty);
    *lhs = new_node_cast(*lhs, ty);
    *rhs = new_node_cast(*rhs, ty);
}

void add_type_binop(Node *node) {
    add_type(node->binop.lhs);
    add_type(node->binop.rhs);
}

void add_type_init(Node *node) {
    switch (node->kind) {
    case ND_COMMA:
        add_type_init(node->binop.lhs);
        add_type_init(node->binop.rhs);
        node->ty = node->binop.rhs->ty;
        return;
    case ND_ASSIGN:
        add_type_init(node->binop.lhs);
        add_type_init(node->binop.rhs);
        if (!is_type_of(TY_STRUCT, node->binop.lhs->ty)) {
            node->binop.rhs =
                new_node_cast(node->binop.rhs, node->binop.lhs->ty);
        }
        node->ty = node->binop.lhs->ty;
        return;
    default:
        add_type(node);
    }
}

void add_type(Node *node) {
    if (!node || node->ty) {
        return;
    }

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITOR:
    case ND_BITXOR:
        add_type_binop(node);
        usual_arith_conv(&node->binop.lhs, &node->binop.rhs);
        node->ty = node->binop.lhs->ty;
        return;
    case ND_ASSIGN:
        add_type_binop(node);
        if (node->binop.lhs->ty->is_const) {
            error_tok(node->binop.lhs->tok, "読み取り専用の変数です");
        }
        if (is_type_of(TY_ARY, node->binop.lhs->ty)) {
            error_tok(node->binop.lhs->tok, "左辺値ではありません");
        }
        if (!is_type_of(TY_STRUCT, node->binop.lhs->ty)) {
            node->binop.rhs =
                new_node_cast(node->binop.rhs, node->binop.lhs->ty);
        }
        node->ty = node->binop.lhs->ty;
        return;
    case ND_EQ:
    case ND_LE:
    case ND_LT:
    case ND_NE:
        add_type_binop(node);
        usual_arith_conv(&node->binop.lhs, &node->binop.rhs);
        node->ty = ty_int;
        return;
    case ND_LOGAND:
    case ND_LOGOR:
        add_type_binop(node);
        node->ty = ty_int;
        return;
    case ND_NOT:
        add_type(node->unary.expr);
        node->ty = ty_int;
        return;
    case ND_NUM:
        if (node->num.val < INT32_MIN || INT32_MAX < node->num.val) {
            node->ty = ty_long;
        } else {
            node->ty = ty_int;
        }
        return;
    case ND_FUNCALL:
        node->ty = ty_long;
        for (Node *n = node->funcall.args; n; n = n->next) {
            add_type(n);
        }
        return;
    case ND_VAR:
        node->ty = node->var.var->ty;
        return;
    case ND_BITNOT:
    case ND_SHL:
    case ND_SHR:
        add_type_binop(node);
        node->ty = node->binop.lhs->ty;
        return;
    case ND_ADDR:
        add_type(node->unary.expr);
        if (is_type_of(TY_ARY, node->unary.expr->ty)) {
            node->ty = pointer_to(node->unary.expr->ty->base);
        } else {
            node->ty = pointer_to(node->unary.expr->ty);
        }
        return;
    case ND_DEREF:
        add_type(node->unary.expr);
        if (!node->unary.expr->ty->base) {
            error_tok(node->tok, "参照外しできません");
        }

        if (is_type_of(TY_VOID, node->unary.expr->ty->base)) {
            error_tok(node->tok, "'void *'型の参照外しはできません");
        }

        node->ty = node->unary.expr->ty->base;
        return;
    case ND_COMMA:
        add_type_binop(node);
        node->ty = node->binop.rhs->ty;
        return;
    case ND_COND:
        add_type(node->condop.cond);
        add_type(node->condop.then);
        add_type(node->condop.els);
        if (is_type_of(TY_VOID, node->condop.then->ty) ||
            is_type_of(TY_VOID, node->condop.els->ty)) {
            node->ty = ty_void;
        } else {
            usual_arith_conv(&node->condop.then, &node->condop.els);
            node->ty = node->condop.then->ty;
        }
        return;
    case ND_MEMBER:
        add_type(node->member.lhs);
        node->ty = node->member.mem->ty;
        return;
    case ND_STMT_EXPR:
        if (node->block.body) {
            Node *stmt = node->block.body;
            for (; stmt->next; stmt = stmt->next) {
            }

            if (stmt->kind == ND_EXPR_STMT) {
                node->ty = stmt->exprstmt.expr->ty;
                return;
            }
        }
        error_tok(node->tok, "voidを返すことはできません");
        return;

    case ND_IF:
        add_type(node->if_.cond);
        add_type(node->if_.then);
        add_type(node->if_.els);
        return;
    case ND_SWITCH:
        add_type(node->switch_.cond);
        add_type(node->switch_.then);
        return;
    case ND_CASE:
        add_type(node->case_.stmt);
        return;
    case ND_WHILE:
        add_type(node->while_.cond);
        add_type(node->while_.then);
        return;
    case ND_FOR:
        add_type(node->for_.init);
        add_type(node->for_.cond);
        add_type(node->for_.after);
        add_type(node->for_.then);
        return;
    case ND_LABEL:
        add_type(node->label.stmt);
        return;
    case ND_RETURN:
        add_type(node->return_.expr);
        return;
    case ND_EXPR_STMT:
        add_type(node->exprstmt.expr);
        return;
    case ND_BLOCK:
        for (Node *n = node->block.body; n; n = n->next) {
            add_type(n);
        }
        return;
    case ND_INIT:
        add_type_init(node->init.assigns);
        return;
    default:;
    }
}
