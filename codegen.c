#include "9cc.h"

//
// コード生成部
//

#define RAX "rax"
#define RDI "rdi"
#define RSI "rsi"
#define RDX "rdx"
#define RCX "rcx"
#define R8 "r8"
#define R9 "r9"
#define RBP "rbp"
#define RSP "rsp"
#define RIP "rip"

#define EAX "eax"
#define EDI "edi"
#define ESI "esi"
#define EDX "edx"
#define ECX "ecx"
#define R8D "r8d"
#define R9D "r9d"

#define AX "ax"
#define DI "di"
#define SI "si"
#define DX "dx"
#define CX "cx"
#define R8W "r8w"
#define R9W "r9w"

#define AL "al"
#define DIL "dil"
#define SIL "sil"
#define DL "dl"
#define CL "cl"
#define R8B "r8b"
#define R9B "r9b"

#define BYTE_PTR(memop) format("BYTE PTR %s", memop)
#define WORD_PTR(memop) format("WORD PTR %s", memop)
#define DWORD_PTR(memop) format("DWORD PTR %s", memop)

#define IMM(val) format("%ld", val)
#define INDIRECT(base, disp) format("[%s+%d]", base, disp)
#define INDIRECT_LABEL(label, pc) format("%s[%s]", label, pc)

#define ADD(o1, o2) println("    add %s, %s", o1, o2)
#define AND(o1, o2) println("    and %s, %s", o1, o2)
#define CALL(o1) println("    call %s", o1)
#define CDQ() println("    cdq")
#define CMP(o1, o2) println("    cmp %s, %s", o1, o2)
#define CQO() println("    cqo")
#define DIV(o1) println("    div %s", o1)
#define IDIV(o1) println("    idiv %s", o1)
#define IMUL(o1, o2) println("    imul %s, %s", o1, o2)
#define JE(o1) println("    je %s", o1)
#define JMP(o1) println("    jmp %s", o1)
#define JNE(o1) println("    jne %s", o1)
#define LEA(o1, o2) println("    lea %s, %s", o1, o2)
#define MOV(o1, o2) println("    mov %s, %s", o1, o2)
#define MOVSX(o1, o2) println("    movsx %s, %s", o1, o2)
#define MOVSXD(o1, o2) println("    movsxd %s, %s", o1, o2)
#define MOVZB(o1, o2) println("    movzb %s, %s", o1, o2)
#define MOVZX(o1, o2) println("    movzx %s, %s", o1, o2)
#define NOT(o1) println("    not %s", o1)
#define OR(o1, o2) println("    or %s, %s", o1, o2)
#define POP(o1) println("    pop %s", o1)
#define PUSH(o1) println("    push %s", o1)
#define RET() println("    ret")
#define SAL(o1, o2) println("    sal %s, %s", o1, o2)
#define SAR(o1, o2) println("    sar %s, %s", o1, o2)
#define SHR(o1, o2) println("    shr %s, %s", o1, o2)
#define SETB(o1) println("    setb %s", o1)
#define SETBE(o1) println("    setbe %s", o1)
#define SETE(o1) println("    sete %s", o1)
#define SETL(o1) println("    setl %s", o1)
#define SETLE(o1) println("    setle %s", o1)
#define SETNE(o1) println("    setne %s", o1)
#define SUB(o1, o2) println("    sub %s, %s", o1, o2)
#define XOR(o1, o2) println("    xor %s, %s", o1, o2)

static char *const argreg64[] = {RDI, RSI, RDX, RCX, R8, R9};
static char *const argreg32[] = {EDI, ESI, EDX, ECX, R8D, R9D};
static char *const argreg16[] = {DI, SI, DX, CX, R8W, R9W};
static char *const argreg8[] = {DIL, SIL, DL, CL, R8B, R9B};

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

int count() {
    static int i = 1;
    return i++;
}

char *format(const char *fmt, ...) {
    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fclose(out);
    return buf;
}

static void println(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    putchar('\n');
}

// nをalignの直近の倍数に切り上げる
int align_to(int n, int align) {
    assert(align != 0);
    return (n + align - 1) / align * align;
}

static void load(Type *ty) {
    if (is_type_of(TY_ARY, ty) || is_type_of(TY_STRUCT, ty) ||
        is_type_of(TY_UNION, ty)) {
        return;
    }

    switch (ty->size) {
    case 1:
        if (ty->is_unsigned) {
            MOVZX(EAX, BYTE_PTR(INDIRECT(RAX, 0)));
        } else {
            MOVSX(EAX, BYTE_PTR(INDIRECT(RAX, 0)));
        }
        break;
    case 2:
        if (ty->is_unsigned) {
            MOVZX(EAX, WORD_PTR(INDIRECT(RAX, 0)));
        } else {
            MOVSX(EAX, WORD_PTR(INDIRECT(RAX, 0)));
        }
        break;
    case 4:
        MOV(EAX, DWORD_PTR(INDIRECT(RAX, 0)));
        break;
    case 8:
        MOV(RAX, INDIRECT(RAX, 0));
        break;
    default:;
    }
}

static void store2(int size, int offset) {
    switch (size) {
    case 1:
        MOVSX(EDX, BYTE_PTR(INDIRECT(RDI, offset)));
        MOV(INDIRECT(RAX, offset), DL);
        return;
    case 2:
        MOVSX(EDX, WORD_PTR(INDIRECT(RDI, offset)));
        MOV(INDIRECT(RAX, offset), DX);
        return;
    case 4:
        MOV(EDX, DWORD_PTR(INDIRECT(RDI, offset)));
        MOV(INDIRECT(RAX, offset), EDX);
        return;
    case 8:
        MOV(RDX, INDIRECT(RDI, offset));
        MOV(INDIRECT(RAX, offset), RDX);
        return;
    default:;
    }
}

static void store(Type *ty, int offset) {
    if (is_type_of(TY_STRUCT, ty)) {
        for (Member *mem = ty->members; mem; mem = mem->next) {
            if (is_type_of(TY_STRUCT, mem->ty) ||
                is_type_of(TY_UNION, mem->ty) || is_type_of(TY_ARY, mem->ty)) {
                store(mem->ty, offset + mem->offset);
                continue;
            }

            store2(mem->ty->size, offset + mem->offset);
        }
        return;
    }

    if (is_type_of(TY_UNION, ty)) {
        if (is_type_of(TY_STRUCT, ty->members->ty) ||
            is_type_of(TY_UNION, ty->members->ty) ||
            is_type_of(TY_ARY, ty->members->ty)) {
            store(ty->members->ty, offset);
            return;
        }

        store2(ty->members->ty->size, offset);
        return;
    }

    if (is_type_of(TY_ARY, ty)) {
        for (int i = 0; i < ty->ary_len; i++) {
            store2(ty->base->size, offset + ty->base->size * i);
        }
        return;
    }

    switch (ty->size) {
    case 1:
        MOV(INDIRECT(RAX, 0), DIL);
        break;
    case 2:
        MOV(INDIRECT(RAX, 0), DI);
        break;
    case 4:
        MOV(INDIRECT(RAX, 0), EDI);
        break;
    case 8:
        MOV(INDIRECT(RAX, 0), RDI);
        break;
    default:;
    }
}

static void s_8_32() { MOVSX(EAX, AL); }
static void s_16_32() { MOVSX(EAX, AX); }
static void s_32_64() { MOVSXD(RAX, EAX); }
static void z_8_32() { MOVZX(EAX, AL); }
static void z_16_32() { MOVZX(EAX, AX); }
static void z_32_64() { MOV(EAX, EAX); }
static void nocast() {}

static void (*cast_fn_table[][8])() = {
    {nocast, nocast, nocast, s_32_64, z_8_32, z_16_32, nocast, s_32_64},
    {s_8_32, nocast, nocast, s_32_64, z_8_32, z_16_32, nocast, s_32_64},
    {s_8_32, s_16_32, nocast, s_32_64, z_8_32, z_16_32, nocast, s_32_64},
    {s_8_32, s_16_32, nocast, nocast, z_8_32, z_16_32, nocast, nocast},
    {s_8_32, nocast, nocast, s_32_64, nocast, nocast, nocast, s_32_64},
    {s_8_32, s_16_32, nocast, s_32_64, z_8_32, nocast, nocast, s_32_64},
    {s_8_32, s_16_32, nocast, s_32_64, z_8_32, z_16_32, nocast, z_32_64},
    {s_8_32, s_16_32, nocast, nocast, z_8_32, z_16_32, nocast, nocast},
};

static int type_id(Type *ty) {
    assert(is_integer(ty) || is_type_of(TY_PTR, ty));

    enum { I8, I16, I32, I64, U8, U16, U32, U64 };

    switch (ty->size) {
    case 1:
        return ty->is_unsigned ? U8 : I8;
    case 2:
        return ty->is_unsigned ? U16 : I16;
    case 4:
        return ty->is_unsigned ? U32 : I32;
    case 8:
        return ty->is_unsigned ? U64 : I64;
    }
}

static void cast(Type *from, Type *to) {
    if (is_type_of(TY_ARY, from)) {
        from = pointer_to(from->base);
    }
    if (is_type_of(TY_STRUCT, from) || is_type_of(TY_UNION, from) ||
        is_type_of(TY_VOID, to)) {
        return;
    }

    if (is_integer(from) && is_type_of(TY_BOOL, to)) {
        switch (from->size) {
        case 1:
        case 2:
        case 4:
            CMP(EAX, IMM(0));
            break;
        case 8:
            CMP(RAX, IMM(0));
            break;
        default:;
        }

        SETNE(AL);
        MOVZB(EAX, AL);
        return;
    }

    int from_id = type_id(from);
    int to_id = type_id(to);
    cast_fn_table[from_id][to_id]();
}

static void gen_lval(Node *node) {
    switch (node->kind) {
    case ND_VAR:
        if (node->var.var->kind == LVAR) {
            // ローカル変数
            MOV(RAX, RBP);
            SUB(RAX, IMM(node->var.var->offset));
            PUSH(RAX);
        } else {
            // グローバル変数
            LEA(RAX, INDIRECT_LABEL(node->var.var->name, RIP));
            PUSH(RAX);
        }
        return;
    case ND_MEMBER:
        gen_lval(node->member.lhs);
        POP(RAX);
        ADD(RAX, IMM(node->member.mem->offset));
        PUSH(RAX);
        return;
    case ND_DEREF:
        gen_expr(node->unary.expr);
        return;
    case ND_COMMA:
        gen_expr(node->binop.lhs);
        POP(RAX);
        gen_lval(node->binop.rhs);
        return;
    default:;
    }

    error_tok(node->tok, "代入の左辺値が変数ではありません");
}

static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        if (node->num.val < INT32_MIN || INT32_MAX < node->num.val) {
            MOV(RAX, IMM(node->num.val));
            PUSH(RAX);
        } else {
            PUSH(IMM(node->num.val));
        }
        return;
    case ND_CAST:
        gen_expr(node->unary.expr);
        POP(RAX);
        cast(node->unary.expr->ty, node->ty);
        PUSH(RAX);
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_lval(node);
        POP(RAX);
        load(node->ty);
        PUSH(RAX);
        return;
    case ND_ADDR:
        gen_lval(node->unary.expr);
        return;
    case ND_DEREF:
        gen_expr(node->unary.expr);
        POP(RAX);
        load(node->ty);
        PUSH(RAX);
        return;
    case ND_FUNCALL: {
        int nargs = 0;
        for (Node *arg = node->funcall.args; arg; arg = arg->next) {
            gen_expr(arg);
            nargs++;
        }

        for (int i = nargs - 1; i >= 0; i--) {
            POP(argreg64[i]);
        }

        MOV(RAX, IMM(0));
        CALL(node->funcall.fn->name);
        PUSH(RAX);
        return;
    }
    case ND_ASSIGN:
        gen_lval(node->binop.lhs);
        gen_expr(node->binop.rhs);
        POP(RDI);
        POP(RAX);
        store(node->ty, 0);
        PUSH(RDI);
        return;
    case ND_NOT:
        gen_expr(node->unary.expr);

        POP(RAX);
        CMP(RAX, IMM(0));
        SETE(AL);
        MOVZB(RAX, AL);
        PUSH(RAX);
        return;
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->binop.lhs);
        POP(RAX);
        CMP(RAX, IMM(0));
        JE(format(".Lfalse%d", c));
        gen_expr(node->binop.rhs);
        POP(RAX);
        CMP(RAX, IMM(0));
        JE(format(".Lfalse%d", c));
        MOV(RAX, IMM(1));
        JMP(format(".Lend%d", c));
        println(".Lfalse%d:", c);
        MOV(RAX, IMM(0));
        println(".Lend%d:", c);
        PUSH(RAX);
        return;
    }
    case ND_LOGOR: {
        int c = count();
        gen_expr(node->binop.lhs);
        POP(RAX);
        CMP(RAX, IMM(0));
        JNE(format(".Ltrue%d", c));
        gen_expr(node->binop.rhs);
        POP(RAX);
        CMP(RAX, IMM(0));
        JNE(format(".Ltrue%d", c));
        MOV(RAX, IMM(0));
        JMP(format(".Lend%d", c));
        println(".Ltrue%d:", c);
        MOV(RAX, IMM(1));
        println(".Lend%d:", c);
        PUSH(RAX);
        return;
    }
    case ND_BITNOT:
        gen_expr(node->unary.expr);
        POP(RAX);
        NOT(RAX);
        PUSH(RAX);
        return;
    case ND_COMMA:
        gen_expr(node->binop.lhs);
        POP(RAX);
        gen_expr(node->binop.rhs);
        return;
    case ND_COND: {
        int c = count();
        gen_expr(node->condop.cond);
        POP(RAX);
        CMP(RAX, IMM(0));
        JE(format(".Lelse%d", c));
        gen_expr(node->condop.then);
        POP(RAX);
        JMP(format(".Lend%d", c));
        println(".Lelse%d:", c);
        gen_expr(node->condop.els);
        POP(RAX);
        println(".Lend%d:", c);
        PUSH(RAX);
        return;
    }
    case ND_STMT_EXPR:
        for (Node *n = node->block.body; n; n = n->next) {
            gen_stmt(n);
        }
        PUSH(RAX);
        return;
    case ND_NULL_EXPR:
        PUSH(RAX);
        return;
    default:;
    }

    gen_expr(node->binop.lhs);
    gen_expr(node->binop.rhs);

    POP(RDI);
    POP(RAX);

    char *ax = node->binop.lhs->ty->kind == TY_LONG || node->binop.lhs->ty->base
                   ? RAX
                   : EAX;
    char *di = node->binop.lhs->ty->kind == TY_LONG || node->binop.lhs->ty->base
                   ? RDI
                   : EDI;
    char *dx = node->binop.lhs->ty->kind == TY_LONG || node->binop.lhs->ty->base
                   ? RDX
                   : EDX;

    switch (node->kind) {
    case ND_ADD:
        ADD(ax, di);
        break;
    case ND_SUB:
        SUB(ax, di);
        break;
    case ND_MUL:
        IMUL(ax, di);
        break;
    case ND_DIV:
    case ND_MOD:
        if (node->ty->is_unsigned) {
            MOV(dx, IMM(0));
            DIV(di);
        } else {
            node->binop.lhs->ty->size == 8 ? CQO() : CDQ();
            IDIV(di);
        }

        if (node->kind == ND_MOD) {
            MOV(RAX, RDX);
        }
        break;
    case ND_EQ:
        CMP(ax, di);
        SETE(AL);
        MOVZB(RAX, AL);
        break;
    case ND_NE:
        CMP(ax, di);
        SETNE(AL);
        MOVZB(RAX, AL);
        break;
    case ND_LT:
        CMP(ax, di);
        node->binop.lhs->ty->is_unsigned ? SETB(AL) : SETL(AL);
        MOVZB(RAX, AL);
        break;
    case ND_LE:
        CMP(ax, di);
        node->binop.lhs->ty->is_unsigned ? SETBE(AL) : SETLE(AL);
        MOVZB(RAX, AL);
        break;
    case ND_BITAND:
        AND(ax, di);
        break;
    case ND_BITOR:
        OR(ax, di);
        break;
    case ND_BITXOR:
        XOR(ax, di);
        break;
    case ND_SHL:
        MOV(ECX, EDI);
        SAL(ax, CL);
        break;
    case ND_SHR:
        MOV(ECX, EDI);
        node->ty->is_unsigned ? SHR(ax, CL) : SAR(ax, CL);
        break;
    default:;
    }

    PUSH(RAX);
}

static void gen_stmt(Node *node) {
    if (node->tok) {
        println("    .loc 1 %d %d", node->tok->line_no, node->tok->column_no);
    }

    switch (node->kind) {
    case ND_BLOCK: {
        for (Node *cur = node->block.body; cur; cur = cur->next) {
            gen_stmt(cur);
        }

        return;
    }
    case ND_GOTO:
        JMP(format(".L%d", node->goto_.id));
        return;
    case ND_LABEL:
        println(".L%d:", node->label.id);
        gen_stmt(node->label.stmt);
        return;
    case ND_RETURN:
        gen_expr(node->return_.expr);

        POP(RAX);
        MOV(RSP, RBP);
        POP(RBP);
        RET();
        return;
    case ND_IF: {
        int c = count();
        gen_expr(node->if_.cond);

        POP(RAX);
        CMP(RAX, IMM(0));
        JE(format(".Lelse%d", c));

        gen_stmt(node->if_.then);

        JMP(format(".Lend%d", c));
        println(".Lelse%d:", c);

        if (node->if_.els) {
            gen_stmt(node->if_.els);
        }

        println(".Lend%d:", c);

        return;
    }
    case ND_SWITCH:
        gen_expr(node->switch_.cond);
        POP(RAX);

        for (Node *n = node->switch_.cases; n; n = n->case_.next) {
            CMP(RAX, IMM(n->case_.val));
            JE(format(".L%d", n->case_.id));
        }

        if (node->switch_.default_) {
            JMP(format(".L%d", node->switch_.default_->case_.id));
        }

        JMP(format(".L%d", node->switch_.brk_label_id));
        gen_stmt(node->switch_.then);
        println(".L%d:", node->switch_.brk_label_id);
        return;
    case ND_CASE:
        println(".L%d:", node->case_.id);
        gen_stmt(node->case_.stmt);
        return;
    case ND_WHILE: {
        int c = count();

        println(".Lbegin%d:", c);

        if (node->while_.cond) {
            gen_expr(node->while_.cond);
            POP(RAX);
            CMP(RAX, IMM(0));
            JE(format(".L%d", node->while_.brk_label_id));
        }

        gen_stmt(node->while_.then);
        println(".L%d:", node->while_.cont_label_id);

        JMP(format(".Lbegin%d", c));
        println(".L%d:", node->while_.brk_label_id);
        return;
    }
    case ND_FOR: {
        int c = count();

        if (node->for_.init) {
            gen_stmt(node->for_.init);
        }

        println(".Lbegin%d:", c);

        if (node->for_.cond) {
            gen_expr(node->for_.cond);
            POP(RAX);
            CMP(RAX, IMM(0));
            JE(format(".L%d", node->for_.brk_label_id));
        }

        gen_stmt(node->for_.then);
        println(".L%d:", node->for_.cont_label_id);
        if (node->for_.after) {
            gen_expr(node->for_.after);
            POP(RAX);
        }

        JMP(format(".Lbegin%d", c));
        println(".L%d:", node->for_.brk_label_id);
        return;
    }
    case ND_INIT:
        gen_expr(node->init.assigns);
        POP(RAX);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->exprstmt.expr);
        POP(RAX);
        return;
    case ND_NULL_STMT:
        return;
    default:
        gen_expr(node);
    }
}

static void emit_gvar_init(Initializer *cur, Initializer *pre) {
    if (pre) {
        int padding = cur->ty->align - pre->ty->align;
        if (padding > 0) {
            println("    .zero %d", padding);
        }
    }

    if (cur->expr) {
        char *label = NULL;
        int64_t val = calc_const_expr(cur->expr, &label);
        if (label) {
            println("    .quad %s", label);
            return;
        }

        switch (cur->ty->size) {
        case 1:
            println("    .byte %ld", val);
            return;
        case 2:
            println("    .value %ld", val);
            return;
        case 4:
            println("    .long %ld", val);
            return;
        case 8:
            println("    .quad %ld", val);
            return;
        default:;
        }
    }

    switch (cur->ty->kind) {
    case TY_ARY:
        emit_gvar_init(cur->children[0], NULL);
        for (int i = 1; i < cur->ty->ary_len; i++) {
            emit_gvar_init(cur->children[i], cur->children[i - 1]);
        }
        break;
    case TY_STRUCT: {
        Member *mem = cur->ty->members;
        emit_gvar_init(cur->children[mem->idx], NULL);
        for (; mem->next; mem = mem->next) {
            emit_gvar_init(cur->children[mem->next->idx],
                           cur->children[mem->idx]);
        }

        int padding = cur->ty->align - mem->ty->align;
        if (padding > 0) {
            println("    .zero %d", padding);
        }
        break;
    }
    case TY_UNION: {
        emit_gvar_init(cur->children[0], NULL);
        int padding = cur->ty->size - cur->children[0]->ty->size;
        if (padding > 0) {
            println("    .zero %d", padding);
        }
        break;
    }
    default:;
    }
}

static void emit_global_variables() {
    if (!globals.next) {
        return;
    }

    for (Obj *var = globals.next; var; var = var->next) {
        if (!var->has_definition) {
            continue;
        }

        if (var->init_data_str) {
            println("    .section .rodata");
            println("%s:", var->name);
            println("    .string \"%s\"", var->init_data_str);
            continue;
        }

        if (!var->is_static) {
            println("    .globl %s", var->name);
        }

        if (var->init) {
            if (var->ty->is_const) {
                println("    .section .rodata");
            } else {
                println("    .data");
            }
            println("%s:", var->name);
            emit_gvar_init(var->init, NULL);
        } else {
            if (var->ty->is_const) {
                println("    .section .rodata");
            } else {
                println("    .bss");
            }
            println("%s:", var->name);
            println("    .zero %d", var->ty->size);
        }
    }
}

static void emit_functions() {
    for (Obj *fn = functions.next; fn; fn = fn->next) {
        if (!fn->has_definition) {
            continue;
        }

        if (fn->locals) {
            int offset = 0;
            for (Obj *var = fn->locals; var; var = var->next) {
                offset += var->ty->size;
                offset = var->offset = align_to(offset, var->ty->align);
            }

            fn->stack_size = align_to(offset, 16);
        }

        // アセンブリの前半部分を出力
        println("    .globl %s", fn->name);
        println("    .text");
        println("%s:", fn->name);
        println("    .loc 1 %d %d", fn->lbrace->line_no, fn->lbrace->column_no);

        // プロローグ
        // 変数の領域を確保する
        PUSH(RBP);
        MOV(RBP, RSP);
        SUB(RSP, IMM(fn->stack_size));

        // レジスタによって渡された引数の値をスタックに保存する
        Obj *var = fn->locals;
        for (int i = 0; i < fn->nparams; i++) {
            switch (var->ty->size) {
            case 1:
                MOV(INDIRECT(RBP, -var->offset), argreg8[i]);
                break;
            case 2:
                MOV(INDIRECT(RBP, -var->offset), argreg16[i]);
                break;
            case 4:
                MOV(INDIRECT(RBP, -var->offset), argreg32[i]);
                break;
            case 8:
                MOV(INDIRECT(RBP, -var->offset), argreg64[i]);
                break;
            default:;
            }

            var = var->next;
        }

        // 先頭の式から順にコード生成
        gen_stmt(fn->body);

        // スタックトップに式全体の値が残っているはずなのでスタックが溢れないようにポップしておく
        POP(RAX);

        // エピローグ
        // 最後の式の結果がRAXに残っているのでそれが戻り値となる
        println("    .loc 1 %d %d", fn->body->tok->line_no,
                fn->body->tok->column_no);
        MOV(RSP, RBP);
        POP(RBP);
        RET();
    }
}

void codegen() {
    println("    .intel_syntax noprefix");
    println("    .file 1 \"%s\"", filepath);

    emit_global_variables();
    emit_functions();
}
