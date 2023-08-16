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
Type *ty_float = &(Type){TY_FLOAT, 4, 4};
Type *ty_double = &(Type){TY_DOUBLE, 8, 8};

// ty->kindが指定したTypeKind(n個)のいずれかと一致するかどうか
bool is_any_type_of(Type *ty, int n, ...) {
    assert(ty);

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++) {
        TypeKind kind = va_arg(ap, TypeKind);
        if (ty->kind == kind) {
            return true;
        }
    }
    return false;
}

bool is_integer(Type *ty) {
    return is_any_type_of(ty, 6, TY_BOOL, TY_CHAR, TY_SHORT, TY_INT, TY_LONG,
                          TY_ENUM);
}

bool is_flonum(Type *ty) { return is_any_type_of(ty, 2, TY_FLOAT, TY_DOUBLE); }

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

Type *func_type(Type *ret, Type *params) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->ret = ret;
    ty->params = params;
    return ty;
}

Member *new_member(int idx, Type *ty, Token *name, int offset) {
    Member *mem = calloc(1, sizeof(Member));
    mem->idx = idx;
    mem->ty = ty;
    mem->name = name;
    mem->offset = offset;
    return mem;
}

// __builtin_va_list型
// struct {
//   unsigned int gp_offset;
//   unsigned int fp_offset;
//   void *overflow_arg_area;
//   void *reg_save_area;
// }[1];
Type *va_list_type() {
    static Type *ty;
    if (!ty) {
        ty = calloc(1, sizeof(Type));
        ty->kind = TY_STRUCT;
        ty->size = 24;
        ty->align = 8;
        ty->members = new_member(0, ty_uint, NULL, 0);
        ty->members->next = new_member(1, ty_uint, NULL, 4);
        ty->members->next->next = new_member(2, pointer_to(ty_void), NULL, 8);
        ty->members->next->next->next =
            new_member(4, pointer_to(ty_void), NULL, 16);
        ty = array_of(ty, 1);
    }
    return ty;
}

void add_const(Type **ty) {
    if (is_any_type_of(*ty, 1, TY_ARY)) {
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
        if (!is_any_type_of(node->binop.lhs->ty, 1, TY_STRUCT)) {
            node->binop.rhs =
                new_node_cast(node->binop.rhs, node->binop.lhs->ty);
        }
        if (is_any_type_of(node->binop.rhs->ty, 1, TY_FUNC)) {
            node->binop.rhs = new_node_unary(ND_ADDR, node->binop.rhs, NULL);
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
        if (is_any_type_of(node->binop.lhs->ty, 1, TY_ARY)) {
            error_tok(node->binop.lhs->tok, "左辺値ではありません");
        }
        if (!is_any_type_of(node->binop.lhs->ty, 1, TY_STRUCT)) {
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
        node->ty = ty_int;
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
        if (is_any_type_of(node->unary.expr->ty, 1, TY_ARY)) {
            node->ty = pointer_to(node->unary.expr->ty->base);
        } else {
            node->ty = pointer_to(node->unary.expr->ty);
        }
        return;
    case ND_DEREF:
        add_type(node->unary.expr);
        if (node->unary.expr->ty->kind == TY_FUNC) {
            node->ty = node->unary.expr->ty;
            return;
        }

        if (!node->unary.expr->ty->base) {
            error_tok(node->tok, "参照外しできません");
        }

        if (is_any_type_of(node->unary.expr->ty->base, 1, TY_VOID)) {
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
        if (is_any_type_of(node->condop.then->ty, 1, TY_VOID) ||
            is_any_type_of(node->condop.els->ty, 1, TY_VOID)) {
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
    case ND_NULL_EXPR:
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
    case ND_DO:
        add_type(node->do_.cond);
        add_type(node->do_.then);
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
    case ND_GOTO:
    case ND_NULL_STMT:
        return;
    default:
        unreachable();
    }
}
