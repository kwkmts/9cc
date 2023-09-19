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
#define R10 "r10"
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

#define XMM0 "xmm0"
#define XMM1 "xmm1"
#define XMM2 "xmm2"
#define XMM3 "xmm3"
#define XMM4 "xmm4"
#define XMM5 "xmm5"
#define XMM6 "xmm6"
#define XMM7 "xmm7"

#define BYTE_PTR(memop) format("BYTE PTR %s", memop)
#define WORD_PTR(memop) format("WORD PTR %s", memop)
#define DWORD_PTR(memop) format("DWORD PTR %s", memop)

#define IMM(val) format("%ld", val)
#define INDIRECT(base, disp) format("[%s+%d]", base, disp)
#define INDIRECT_LABEL(label, pc) format("%s[%s]", label, pc)

#define ADD(o1, o2) println("    add %s, %s", o1, o2)
#define ADDSD(o1, o2) println("    addsd %s, %s", o1, o2)
#define ADDSS(o1, o2) println("    addss %s, %s", o1, o2)
#define AND(o1, o2) println("    and %s, %s", o1, o2)
#define CALL(o1) println("    call %s", o1)
#define CDQ() println("    cdq")
#define CMP(o1, o2) println("    cmp %s, %s", o1, o2)
#define CQO() println("    cqo")
#define CVTSD2SS(o1, o2) println("    cvtsd2ss %s, %s", o1, o2)
#define CVTSI2SD(o1, o2) println("    cvtsi2sd %s, %s", o1, o2)
#define CVTSI2SS(o1, o2) println("    cvtsi2ss %s, %s", o1, o2)
#define CVTSS2SD(o1, o2) println("    cvtss2sd %s, %s", o1, o2)
#define CVTTSD2SI(o1, o2) println("    cvttsd2si %s, %s", o1, o2)
#define CVTTSS2SI(o1, o2) println("    cvttss2si %s, %s", o1, o2)
#define DIV(o1) println("    div %s", o1)
#define DIVSD(o1, o2) println("    divsd %s, %s", o1, o2)
#define DIVSS(o1, o2) println("    divss %s, %s", o1, o2)
#define IDIV(o1) println("    idiv %s", o1)
#define IMUL(o1, o2) println("    imul %s, %s", o1, o2)
#define JAE(o1) println("    jae %s", o1)
#define JE(o1) println("    je %s", o1)
#define JMP(o1) println("    jmp %s", o1)
#define JNE(o1) println("    jne %s", o1)
#define JS(o1) println("    js %s", o1)
#define LEA(o1, o2) println("    lea %s, %s", o1, o2)
#define MOV(o1, o2) println("    mov %s, %s", o1, o2)
#define MOVAPS(o1, o2) println("    movaps %s, %s", o1, o2)
#define MOVQ(o1, o2) println("    movq %s, %s", o1, o2)
#define MOVSD(o1, o2) println("    movsd %s, %s", o1, o2)
#define MOVSS(o1, o2) println("    movss %s, %s", o1, o2)
#define MOVSX(o1, o2) println("    movsx %s, %s", o1, o2)
#define MOVSXD(o1, o2) println("    movsxd %s, %s", o1, o2)
#define MOVZB(o1, o2) println("    movzb %s, %s", o1, o2)
#define MOVZX(o1, o2) println("    movzx %s, %s", o1, o2)
#define MULSD(o1, o2) println("    mulsd %s, %s", o1, o2)
#define MULSS(o1, o2) println("    mulss %s, %s", o1, o2)
#define NOT(o1) println("    not %s", o1)
#define OR(o1, o2) println("    or %s, %s", o1, o2)
#define POP(o1) println("    pop %s", o1)
#define PUSH(o1) println("    push %s", o1)
#define PXOR(o1, o2) println("    pxor %s, %s", o1, o2)
#define RET() println("    ret")
#define SAL(o1, o2) println("    sal %s, %s", o1, o2)
#define SAR(o1, o2) println("    sar %s, %s", o1, o2)
#define SHR(o1, o2) println("    shr %s, %s", o1, o2)
#define SETA(o1) println("    seta %s", o1)
#define SETAE(o1) println("    setae %s", o1)
#define SETB(o1) println("    setb %s", o1)
#define SETBE(o1) println("    setbe %s", o1)
#define SETE(o1) println("    sete %s", o1)
#define SETL(o1) println("    setl %s", o1)
#define SETLE(o1) println("    setle %s", o1)
#define SETNE(o1) println("    setne %s", o1)
#define SETNP(o1) println("    setnp %s", o1)
#define SETP(o1) println("    setp %s", o1)
#define SUB(o1, o2) println("    sub %s, %s", o1, o2)
#define SUBSD(o1, o2) println("    subsd %s, %s", o1, o2)
#define SUBSS(o1, o2) println("    subss %s, %s", o1, o2)
#define TEST(o1, o2) println("    test %s, %s", o1, o2)
#define UCOMISD(o1, o2) println("    ucomisd %s, %s", o1, o2)
#define UCOMISS(o1, o2) println("    ucomiss %s, %s", o1, o2)
#define XOR(o1, o2) println("    xor %s, %s", o1, o2)

#define GP_MAX 6
#define FP_MAX 8

static char *const argreg64[] = {RDI, RSI, RDX, RCX, R8, R9};
static char *const argreg32[] = {EDI, ESI, EDX, ECX, R8D, R9D};
static char *const argreg16[] = {DI, SI, DX, CX, R8W, R9W};
static char *const argreg8[] = {DIL, SIL, DL, CL, R8B, R9B};
static char *const argregf[] = {XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7};

static Obj *cur_fn; // 現在コード生成中の関数

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

// プロローグ直後のスタックの状態を基準としてスタックの深さを保持(call と ret
// は除く)
static int depth;

static void push(char *reg) {
    PUSH(reg);
    depth++;
}

static void pop(char *reg) {
    POP(reg);
    depth--;
}

static void pushf(char *reg) {
    SUB(RSP, IMM(8));
    MOVSD(INDIRECT(RSP, 0), reg);
    depth++;
}

static void popf(char *reg) {
    MOVSD(reg, INDIRECT(RSP, 0));
    ADD(RSP, IMM(8));
    depth--;
}

static void cmp_zero(Type *ty) {
    if (is_integer(ty) || ty->kind == TY_PTR) {
        ty->size < 4 ? CMP(EAX, IMM(0)) : CMP(RAX, IMM(0));
        return;
    }

    PXOR(XMM1, XMM1);
    if (ty->kind == TY_FLOAT) {
        UCOMISS(XMM0, XMM1);
        return;
    }
    if (ty->kind == TY_DOUBLE) {
        UCOMISD(XMM0, XMM1);
        return;
    }

    unreachable();
}

static void load(Type *ty) {
    if (is_any_type_of(ty, 5, TY_ARY, TY_STRUCT, TY_UNION, TY_FUNC,
                       TY_VA_LIST)) {
        return;
    }

    if (is_any_type_of(ty, 1, TY_FLOAT)) {
        MOVSS(XMM0, INDIRECT(RAX, 0));
        return;
    }
    if (is_any_type_of(ty, 1, TY_DOUBLE)) {
        MOVSD(XMM0, INDIRECT(RAX, 0));
        return;
    }

    switch (ty->size) {
    case 1:
        if (ty->is_unsigned) {
            MOVZX(EAX, BYTE_PTR(INDIRECT(RAX, 0)));
        } else {
            MOVSX(EAX, BYTE_PTR(INDIRECT(RAX, 0)));
        }
        return;
    case 2:
        if (ty->is_unsigned) {
            MOVZX(EAX, WORD_PTR(INDIRECT(RAX, 0)));
        } else {
            MOVSX(EAX, WORD_PTR(INDIRECT(RAX, 0)));
        }
        return;
    case 4:
        MOV(EAX, DWORD_PTR(INDIRECT(RAX, 0)));
        return;
    case 8:
        MOV(RAX, INDIRECT(RAX, 0));
        return;
    }

    unreachable();
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
    default:
        unreachable();
    }
}

static void store(Type *ty, int offset) {
    if (is_any_type_of(ty, 1, TY_STRUCT)) {
        for (Member *mem = ty->members; mem; mem = mem->next) {
            if (is_any_type_of(mem->ty, 3, TY_STRUCT, TY_UNION, TY_ARY)) {
                store(mem->ty, offset + mem->offset);
                continue;
            }

            store2(mem->ty->size, offset + mem->offset);
        }
        return;
    }

    if (is_any_type_of(ty, 1, TY_UNION)) {
        if (is_any_type_of(ty->members->ty, 3, TY_STRUCT, TY_UNION, TY_ARY)) {
            store(ty->members->ty, offset);
            return;
        }

        store2(ty->members->ty->size, offset);
        return;
    }

    if (is_any_type_of(ty, 1, TY_ARY)) {
        for (int i = 0; i < ty->ary_len; i++) {
            store2(ty->base->size, offset + ty->base->size * i);
        }
        return;
    }

    if (is_any_type_of(ty, 1, TY_FLOAT)) {
        MOVSS(INDIRECT(RAX, 0), XMM0);
        return;
    }
    if (is_any_type_of(ty, 1, TY_DOUBLE)) {
        MOVSD(INDIRECT(RAX, 0), XMM0);
        return;
    }

    switch (ty->size) {
    case 1:
        MOV(INDIRECT(RAX, 0), DIL);
        return;
    case 2:
        MOV(INDIRECT(RAX, 0), DI);
        return;
    case 4:
        MOV(INDIRECT(RAX, 0), EDI);
        return;
    case 8:
        MOV(INDIRECT(RAX, 0), RDI);
        return;
    }

    unreachable();
}

// clang-format off

// キャストに使われる関数群
// [s|z|c][_|f][8|16|32|64][_|f][8|16|32|64]

static void s_8_32() { MOVSX(EAX, AL); }
static void s_8f32() { CVTTSS2SI(EAX, XMM0); s_8_32(); }
static void s_8f64() { CVTTSD2SI(EAX, XMM0); s_8_32(); }
static void s_16_32() { MOVSX(EAX, AX); }
static void s_16f32() { CVTTSS2SI(EAX, XMM0); s_16_32(); }
static void s_16f64() { CVTTSD2SI(EAX, XMM0); s_16_32(); }
static void c_32f32() { CVTTSS2SI(EAX, XMM0); }
static void c_32f64() { CVTTSD2SI(EAX, XMM0); }
static void s_64_32() { MOVSXD(RAX, EAX); }
static void c_64f32() { CVTTSS2SI(RAX, XMM0); }
static void c_64f64() { CVTTSD2SI(RAX, XMM0); }
static void z_8_32() { MOVZX(EAX, AL); }
static void z_8f32() { CVTTSS2SI(EAX, XMM0); z_8_32(); }
static void z_8f64() { CVTTSD2SI(EAX, XMM0); z_8_32(); }
static void z_16_32() { MOVZX(EAX, AX); }
static void z_16f32() { CVTTSS2SI(EAX, XMM0); z_16_32(); }
static void z_16f64() { CVTTSD2SI(EAX, XMM0); z_16_32(); }
static void z_32f32() { CVTTSS2SI(RAX, XMM0); }
static void z_32f64() { CVTTSD2SI(RAX, XMM0); }
static void z_64_32() { MOV(EAX, EAX); }
static void sf32_8() { s_8_32(); CVTSI2SS(XMM0, EAX); }
static void sf32_16() { s_16_32(); CVTSI2SS(XMM0, EAX); }
static void cf32_32() { CVTSI2SS(XMM0, EAX); }
static void cf32_64asI() { CVTSI2SS(XMM0, RAX); }
static void zf32_8() { z_8_32(); CVTSI2SS(XMM0, EAX); }
static void zf32_16() { z_16_32(); CVTSI2SS(XMM0, EAX); }
static void zf32_32() { z_64_32(); CVTSI2SS(XMM0, RAX); }
static void cf32_64asU() {
    int c = count();
    int c2 = count();
    TEST(RAX, RAX);
    JS(format(".L%d", c));
    PXOR(XMM0, XMM0);
    CVTSI2SS(XMM0, RAX);
    JMP(format(".L%d", c2));
    println(".L%d:", c);
    MOV(RDX, RAX);
    SHR(RDX, IMM(1));
    AND(EAX, IMM(1));
    OR(RDX, RAX);
    PXOR(XMM0, XMM0);
    CVTSI2SS(XMM0, RDX);
    ADDSS(XMM0, XMM0);
    println(format(".L%d:", c2));
}
static void cf32f64() { CVTSD2SS(XMM0, XMM0); }
static void sf64_8() { s_8_32(); CVTSI2SD(XMM0, EAX); }
static void sf64_16() { s_16_32(); CVTSI2SD(XMM0, EAX); }
static void cf64_32() { CVTSI2SD(XMM0, EAX); }
static void cf64_64asI() { CVTSI2SD(XMM0, RAX); }
static void zf64_8() { z_8_32(); CVTSI2SD(XMM0, EAX); }
static void zf64_16() { z_16_32(); CVTSI2SD(XMM0, EAX); }
static void zf64_32() { z_64_32(); CVTSI2SD(XMM0, RAX); }
static void cf64_64asU() {
    int c = count();
    int c2 = count();
    TEST(RAX, RAX);
    JS(format(".L%d", c));
    PXOR(XMM0, XMM0);
    CVTSI2SD(XMM0, RAX);
    JMP(format(".L%d", c2));
    println(".L%d:", c);
    MOV(RDX, RAX);
    SHR(RDX, IMM(1));
    AND(EAX, IMM(1));
    OR(RDX, RAX);
    PXOR(XMM0, XMM0);
    CVTSI2SD(XMM0, RDX);
    ADDSD(XMM0, XMM0);
    println(format(".L%d:", c2));
}
static void cf64f32() { CVTSS2SD(XMM0, XMM0); }
static void nocast() {}

static void (*cast_fn_table[][10])(void) = {
  // to i8      i16      i32      i64      u8      u16      u32      u64      f32      f64     // from
    {nocast, nocast,  nocast,  s_64_32, z_8_32, z_16_32, nocast,  s_64_32, sf32_8,     sf64_8    }, // i8
    {s_8_32, nocast,  nocast,  s_64_32, z_8_32, z_16_32, nocast,  s_64_32, sf32_16,    sf64_16   }, // i16
    {s_8_32, s_16_32, nocast,  s_64_32, z_8_32, z_16_32, nocast,  s_64_32, cf32_32,    cf64_32   }, // i32
    {s_8_32, s_16_32, nocast,  nocast,  z_8_32, z_16_32, nocast,  nocast,  cf32_64asI, cf64_64asI}, // i64
    {s_8_32, nocast,  nocast,  s_64_32, nocast, nocast,  nocast,  s_64_32, zf32_8,     zf64_8    }, // u8
    {s_8_32, s_16_32, nocast,  s_64_32, z_8_32, nocast,  nocast,  s_64_32, zf32_16,    zf64_16   }, // u16
    {s_8_32, s_16_32, nocast,  z_64_32, z_8_32, z_16_32, nocast,  z_64_32, zf32_32,    zf64_32   }, // u32
    {s_8_32, s_16_32, nocast,  nocast,  z_8_32, z_16_32, nocast,  nocast,  cf32_64asU, cf64_64asU}, // u64
    {s_8f32, s_16f32, c_32f32, c_64f32, z_8f32, z_16f32, z_32f32, c_64f32, nocast,     cf64f32   }, // f32
    {s_8f64, s_16f64, c_32f64, c_64f64, z_8f64, z_16f64, z_32f64, c_64f64, cf32f64,    nocast    }, // f64
};

// clang-format on

static int type_id(Type *ty) {
    assert(is_integer(ty) || is_flonum(ty) || is_any_type_of(ty, 1, TY_PTR));

    enum { I8, I16, I32, I64, U8, U16, U32, U64, F32, F64 };

    if (is_flonum(ty)) {
        return ty->size == 4 ? F32 : F64;
    }

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

    unreachable();
}

static void cast(Type *from, Type *to) {
    if (is_any_type_of(from, 1, TY_ARY)) {
        from = pointer_to(from->base);
    }
    if (is_any_type_of(from, 4, TY_STRUCT, TY_UNION, TY_FUNC, TY_VA_LIST) ||
        is_any_type_of(to, 1, TY_VOID)) {
        return;
    }

    if (is_integer(from) && is_any_type_of(to, 1, TY_BOOL)) {
        cmp_zero(from);
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
            MOV(RAX, RBP);
            SUB(RAX, IMM((long)node->var.var->offset));
            return;
        }

        if (node->var.var->kind == GVAR) {
            LEA(RAX, INDIRECT_LABEL(node->var.var->name, RIP));
            return;
        }

        if (node->var.var->kind == FUNC) {
            if (node->var.var->has_definition) {
                LEA(RAX, INDIRECT_LABEL(node->var.var->name, RIP));
            } else {
                LEA(RAX,
                    INDIRECT_LABEL(format("%s@PLT", node->var.var->name), RIP));
            }
            return;
        }
    case ND_MEMBER:
        gen_lval(node->member.lhs);
        ADD(RAX, IMM(node->member.mem->offset));
        return;
    case ND_DEREF:
        gen_expr(node->unary.expr);
        return;
    case ND_COMMA:
        gen_expr(node->binop.lhs);
        gen_lval(node->binop.rhs);
        return;
    default:
        error_tok(node->tok, "代入の左辺値が変数ではありません");
    }
}

static void gen_builtin_funcall(Node *node) {
    if (!strcmp(node->funcall.fn->var.var->name, "__builtin_va_start")) {
        int stack = 0;
        int iparam = 0;
        int fparam = 0;
        for (Obj *param = cur_fn->locals; iparam + fparam < cur_fn->nparams;
             param = param->next) {
            if (is_flonum(param->ty)) {
                if (fparam >= FP_MAX) {
                    stack++;
                }
                fparam++;
                continue;
            }

            if (iparam >= GP_MAX) {
                stack++;
            }
            iparam++;
        }

        Node *ap = node->funcall.args;
        int ap_off = -ap->var.var->offset;
        int gp_offset = 8 * iparam;
        int fp_offset = 48 + 16 * fparam;
        int reg_save_area_off = -cur_fn->reg_save_area->offset;
        int overflow_arg_area_off = 16 + stack * 8;

        MOV(DWORD_PTR(INDIRECT(RBP, ap_off)), IMM(gp_offset));
        MOV(DWORD_PTR(INDIRECT(RBP, ap_off + 4)), IMM(fp_offset));
        LEA(RAX, INDIRECT(RBP, overflow_arg_area_off));
        MOV(INDIRECT(RBP, ap_off + 8), RAX);
        LEA(RAX, INDIRECT(RBP, reg_save_area_off));
        MOV(INDIRECT(RBP, ap_off + 16), RAX);

        return;
    }

    if (!strcmp(node->funcall.fn->var.var->name, "__builtin_va_arg")) {
        Node *ap = node->funcall.args;
        int ap_off = -ap->var.var->offset;
        bool f = is_flonum(node->ty);

        // 引数レジスタを消費しきったかどうか
        if (f) {
            MOV(EAX, DWORD_PTR(INDIRECT(RBP, ap_off + 4)));
            CMP(EAX, IMM(176));
        } else {
            MOV(EAX, DWORD_PTR(INDIRECT(RBP, ap_off)));
            CMP(EAX, IMM(48));
        }
        int c = count();
        JAE(format(".L%d", c));

        // 引数レジスタがまだ残っている場合
        // 次に使用可能な引数レジスタのアドレスを計算
        MOV(RAX, INDIRECT(RBP, ap_off + 16));
        f ? MOV(EDX, DWORD_PTR(INDIRECT(RBP, ap_off + 4)))
          : MOV(EDX, DWORD_PTR(INDIRECT(RBP, ap_off)));
        MOV(EDX, EDX);
        ADD(RAX, RDX);

        // gp_offset/fp_offsetを更新
        if (f) {
            MOV(EDX, DWORD_PTR(INDIRECT(RBP, ap_off + 4)));
            ADD(EDX, IMM(16));
            MOV(DWORD_PTR(INDIRECT(RBP, ap_off + 4)), EDX);
        } else {
            MOV(EDX, DWORD_PTR(INDIRECT(RBP, ap_off)));
            ADD(EDX, IMM(8));
            MOV(DWORD_PTR(INDIRECT(RBP, ap_off)), EDX);
        }

        int c2 = count();
        JMP(format(".L%d", c2));

        // 引数レジスタを消費しきった場合
        println(".L%d:", c);
        // overflow_arg_areaを更新
        MOV(RAX, INDIRECT(RBP, ap_off + 8));
        LEA(RDX, INDIRECT(RAX, 8));
        MOV(INDIRECT(RBP, ap_off + 8), RDX);

        println(".L%d:", c2);
        f ? MOVSD(XMM0, INDIRECT(RAX, 0)) : MOV(RAX, INDIRECT(RAX, 0));
        return;
    }

    if (!strcmp(node->funcall.fn->var.var->name, "__builtin_va_end")) {
        return;
    }

    if (!strcmp(node->funcall.fn->var.var->name, "__builtin_va_copy")) {
        Node *dst = node->funcall.args;
        Node *src = dst->next;
        int dst_offset = dst->var.var->offset;
        int src_offset = src->var.var->offset;
        LEA(RCX, INDIRECT(RBP, -dst_offset));
        LEA(RSI, INDIRECT(RBP, -src_offset));
        MOV(RAX, INDIRECT(RSI, 0));
        MOV(INDIRECT(RCX, 0), RAX);
        MOV(RAX, INDIRECT(RSI, 8));
        MOV(INDIRECT(RCX, 8), RAX);
        MOV(RAX, INDIRECT(RSI, 16));
        MOV(INDIRECT(RCX, 16), RAX);
        return;
    }

    unreachable();
}

typedef struct ListNode ListNode;

struct ListNode {
    void *val;
    ListNode *next;
};

// 単方向リストの試験的実装
struct List {
    ListNode *head;
    ListNode *tail;
    int size;
};

List list_new() {
    List list = calloc(1, sizeof(struct List));
    return list;
}

void list_push_front(List list, void *val) {
    ListNode *node = calloc(1, sizeof(ListNode));
    node->val = val;
    list->size++;
    if (!list->head) {
        list->head = node;
        list->tail = node;
        return;
    }
    node->next = list->head;
    list->head = node;
}

void list_push_back(List list, void *val) {
    ListNode *node = calloc(1, sizeof(ListNode));
    node->val = val;
    list->size++;
    if (!list->head) {
        list->head = node;
        list->tail = node;
        return;
    }
    list->tail = list->tail->next = node;
}

ListIter list_begin(List list) { return (ListIter)list->head; }

ListIter list_next(ListIter it) { return (ListIter)((ListNode *)it)->next; }

int list_size(List list) { return list->size; }

static int push_args(Node *args, List *reg_iargs, List *reg_fargs) {
    int stack = 0, iarg = 0, farg = 0;
    List stack_args = list_new();

    for (Node *arg = args; arg; arg = arg->next) {
        if (is_flonum(arg->ty)) {
            if (farg < FP_MAX) {
                list_push_front(*reg_fargs, arg);
            } else {
                list_push_front(stack_args, arg);
                stack++;
            }
            farg++;
            continue;
        }

        if (iarg < GP_MAX) {
            list_push_front(*reg_iargs, arg);
        } else {
            list_push_front(stack_args, arg);
            stack++;
        }
        iarg++;
    }

    // call直前のrspが16バイト境界になるように調整
    if ((depth + stack) % 2) {
        SUB(RSP, IMM(8));
        depth++;
        stack++;
    }

    for (ListIter it = list_begin(stack_args); it; it = list_next(it)) {
        Node *arg = *it;
        gen_expr(arg);

        if (is_flonum(arg->ty)) {
            pushf(XMM0);
            continue;
        }

        push(RAX);
    }

    for (ListIter it = list_begin(*reg_iargs); it; it = list_next(it)) {
        Node *arg = *it;
        gen_expr(arg);
        push(RAX);
    }

    for (ListIter it = list_begin(*reg_fargs); it; it = list_next(it)) {
        Node *arg = *it;
        gen_expr(arg);
        pushf(XMM0);
    }

    return stack;
}

// 式の評価結果をraxまたはxmm0に格納する
static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM: {
        union {
            float f32;
            double f64;
            uint32_t u32;
            uint64_t u64;
        } u;

        switch (node->ty->kind) {
        case TY_FLOAT:
            u.f32 = (float)node->num.fval;
            MOV(EAX, format("%u", u.u32));
            MOVQ(XMM0, RAX);
            return;
        case TY_DOUBLE:
            u.f64 = node->num.fval;
            MOV(RAX, format("%lu", u.u64));
            MOVQ(XMM0, RAX);
            return;
        default:
            MOV(RAX, IMM(node->num.ival));
            return;
        }
    }
    case ND_CAST:
        gen_expr(node->unary.expr);
        cast(node->unary.expr->ty, node->ty);
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_lval(node);
        load(node->ty);
        return;
    case ND_ADDR:
        gen_lval(node->unary.expr);
        return;
    case ND_DEREF:
        gen_expr(node->unary.expr);
        load(node->ty);
        return;
    case ND_FUNCALL: {
        if (node->funcall.fn->kind == ND_VAR &&
            is_builtin(node->funcall.fn->var.var->name)) {
            gen_builtin_funcall(node);
            return;
        }

        List reg_iargs = list_new();
        List reg_fargs = list_new();
        int stack = push_args(node->funcall.args, &reg_iargs, &reg_fargs);

        gen_expr(node->funcall.fn);
        MOV(R10, RAX);

        for (int i = 0; i < list_size(reg_fargs); i++) {
            popf(argregf[i]);
        }
        for (int i = 0; i < list_size(reg_iargs); i++) {
            pop(argreg64[i]);
        }

        if (node->funcall.fn->ty->is_variadic) {
            MOV(EAX, IMM(MIN(list_size(reg_fargs), FP_MAX)));
        }
        CALL(R10);
        if (stack) {
            ADD(RSP, IMM(stack * 8));
            depth -= stack;
        }
        return;
    }
    case ND_ASSIGN: {
        bool f = is_flonum(node->ty);
        gen_expr(node->binop.rhs);
        f ? pushf(XMM0) : push(RAX);
        gen_lval(node->binop.lhs);
        f ? popf(XMM0) : pop(RDI);
        store(node->ty, 0);
        if (!f) {
            MOV(RAX, RDI);
        }
        return;
    }
    case ND_NOT:
        gen_expr(node->unary.expr);
        cmp_zero(node->unary.expr->ty);
        SETE(AL);
        MOVZB(RAX, AL);
        return;
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->binop.lhs);
        cmp_zero(node->binop.lhs->ty);
        JE(format(".Lfalse%d", c));
        gen_expr(node->binop.rhs);
        cmp_zero(node->binop.rhs->ty);
        JE(format(".Lfalse%d", c));
        MOV(RAX, IMM(1));
        JMP(format(".Lend%d", c));
        println(".Lfalse%d:", c);
        MOV(RAX, IMM(0));
        println(".Lend%d:", c);
        return;
    }
    case ND_LOGOR: {
        int c = count();
        gen_expr(node->binop.lhs);
        cmp_zero(node->binop.lhs->ty);
        JNE(format(".Ltrue%d", c));
        gen_expr(node->binop.rhs);
        cmp_zero(node->binop.rhs->ty);
        JNE(format(".Ltrue%d", c));
        MOV(RAX, IMM(0));
        JMP(format(".Lend%d", c));
        println(".Ltrue%d:", c);
        MOV(RAX, IMM(1));
        println(".Lend%d:", c);
        return;
    }
    case ND_BITNOT:
        gen_expr(node->unary.expr);
        NOT(RAX);
        return;
    case ND_COMMA:
        gen_expr(node->binop.lhs);
        gen_expr(node->binop.rhs);
        return;
    case ND_COND: {
        int c = count();
        gen_expr(node->condop.cond);
        cmp_zero(node->condop.cond->ty);
        JE(format(".Lelse%d", c));
        gen_expr(node->condop.then);
        JMP(format(".Lend%d", c));
        println(".Lelse%d:", c);
        gen_expr(node->condop.els);
        println(".Lend%d:", c);
        return;
    }
    case ND_STMT_EXPR:
        for (Node *n = node->block.body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    case ND_NULL_EXPR:
        return;
    default:;
    }

    if (is_flonum(node->binop.lhs->ty)) {
        gen_expr(node->binop.rhs);
        pushf(XMM0);
        gen_expr(node->binop.lhs);
        popf(XMM1);

        bool _32 = node->binop.lhs->ty->kind == TY_FLOAT;
        switch (node->kind) {
        case ND_ADD:
            _32 ? ADDSS(XMM0, XMM1) : ADDSD(XMM0, XMM1);
            return;
        case ND_SUB:
            _32 ? SUBSS(XMM0, XMM1) : SUBSD(XMM0, XMM1);
            return;
        case ND_MUL:
            _32 ? MULSS(XMM0, XMM1) : MULSD(XMM0, XMM1);
            return;
        case ND_DIV:
            _32 ? DIVSS(XMM0, XMM1) : DIVSD(XMM0, XMM1);
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            _32 ? UCOMISS(XMM1, XMM0) : UCOMISD(XMM1, XMM0);

            switch (node->kind) {
            case ND_EQ:
                SETE(AL);
                SETNP(CL);
                AND(AL, CL);
                break;
            case ND_NE:
                SETNE(AL);
                SETP(CL);
                OR(AL, CL);
                break;
            case ND_LT:
                SETA(AL);
                break;
            case ND_LE:
                SETAE(AL);
                break;
            default:
                unreachable();
            }

            AND(AL, IMM(1));
            MOVZX(EAX, AL);
            return;
        default:
            error_tok(node->tok, "不正なオペランド");
        }
    }

    gen_expr(node->binop.rhs);
    push(RAX);
    gen_expr(node->binop.lhs);
    pop(RDI);

    bool _64 =
        node->binop.lhs->ty->kind == TY_LONG || node->binop.lhs->ty->base;
    char *ax = _64 ? RAX : EAX;
    char *di = _64 ? RDI : EDI;
    char *dx = _64 ? RDX : EDX;

    switch (node->kind) {
    case ND_ADD:
        ADD(ax, di);
        return;
    case ND_SUB:
        SUB(ax, di);
        return;
    case ND_MUL:
        IMUL(ax, di);
        return;
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
        return;
    case ND_EQ:
        CMP(ax, di);
        SETE(AL);
        MOVZB(RAX, AL);
        return;
    case ND_NE:
        CMP(ax, di);
        SETNE(AL);
        MOVZB(RAX, AL);
        return;
    case ND_LT:
        CMP(ax, di);
        node->binop.lhs->ty->is_unsigned ? SETB(AL) : SETL(AL);
        MOVZB(RAX, AL);
        return;
    case ND_LE:
        CMP(ax, di);
        node->binop.lhs->ty->is_unsigned ? SETBE(AL) : SETLE(AL);
        MOVZB(RAX, AL);
        return;
    case ND_BITAND:
        AND(ax, di);
        return;
    case ND_BITOR:
        OR(ax, di);
        return;
    case ND_BITXOR:
        XOR(ax, di);
        return;
    case ND_SHL:
        MOV(ECX, EDI);
        SAL(ax, CL);
        return;
    case ND_SHR:
        MOV(ECX, EDI);
        node->ty->is_unsigned ? SHR(ax, CL) : SAR(ax, CL);
        return;
    default:
        unreachable();
    }
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
        JMP(format(".Lreturn.%s", cur_fn->name));
        return;
    case ND_IF: {
        int c = count();
        gen_expr(node->if_.cond);
        cmp_zero(node->if_.cond->ty);
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
        gen_expr(node->while_.cond);
        cmp_zero(node->while_.cond->ty);
        JE(format(".L%d", node->while_.brk_label_id));
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
            cmp_zero(node->for_.cond->ty);
            JE(format(".L%d", node->for_.brk_label_id));
        }

        gen_stmt(node->for_.then);
        println(".L%d:", node->for_.cont_label_id);
        if (node->for_.after) {
            gen_expr(node->for_.after);
        }

        JMP(format(".Lbegin%d", c));
        println(".L%d:", node->for_.brk_label_id);
        return;
    }
    case ND_DO: {
        int c = count();
        println(".Lbegin%d:", c);
        gen_stmt(node->do_.then);
        println(".L%d:", node->do_.cont_label_id);
        gen_expr(node->do_.cond);
        cmp_zero(node->do_.cond->ty);
        JNE(format(".Lbegin%d", c));
        println(".L%d:", node->do_.brk_label_id);
        return;
    }
    case ND_INIT:
        gen_expr(node->init.assigns);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->exprstmt.expr);
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
        }
        unreachable();
    }

    switch (cur->ty->kind) {
    case TY_ARY:
        emit_gvar_init(cur->children[0], NULL);
        for (int i = 1; i < cur->ty->ary_len; i++) {
            emit_gvar_init(cur->children[i], cur->children[i - 1]);
        }
        return;
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
        return;
    }
    case TY_UNION: {
        emit_gvar_init(cur->children[0], NULL);
        int padding = cur->ty->size - cur->children[0]->ty->size;
        if (padding > 0) {
            println("    .zero %d", padding);
        }
        return;
    }
    default:
        unreachable();
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

        // 配列はこの時点で完全な型であることが保証される
        if (var->ty->kind != TY_ARY) {
            var->ty = expand_pseudo(var->pty);
            if (var->ty->size < 0) {
                error_tok(var->tok, "不完全な型の変数宣言です");
            }
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

        cur_fn = fn;

        if (fn->locals) {
            int offset = 0;
            int iparam = 0;
            int fparam = 0;
            int stack = 0;
            Obj *var = fn->locals;

            for (int i = 0; i < fn->nparams; i++, var = var->next) {
                if (is_flonum(var->ty)) {
                    if (fparam < FP_MAX) {
                        offset += var->ty->size;
                        offset = var->offset = align_to(offset, var->ty->align);
                    } else {
                        var->offset = -16 - 8 * stack++;
                    }
                    fparam++;
                    continue;
                }

                if (iparam < GP_MAX) {
                    offset += var->ty->size;
                    offset = var->offset = align_to(offset, var->ty->align);
                } else {
                    var->offset = -16 - 8 * stack++;
                }
                iparam++;
            }

            for (; var; var = var->next) {
                offset += var->ty->size;
                offset = var->offset = align_to(offset, var->ty->align);
            }

            fn->stack_size = align_to(offset, 16);
        }

        // アセンブリの前半部分を出力
        if (!fn->is_static) {
            println("    .globl %s", fn->name);
        }
        println("    .text");
        println("%s:", fn->name);
        println("    .loc 1 %d %d", fn->lbrace->line_no, fn->lbrace->column_no);

        // プロローグ
        // 変数の領域を確保する
        PUSH(RBP);
        MOV(RBP, RSP);
        SUB(RSP, IMM(fn->stack_size));

        if (fn->ty->is_variadic) {
            int offset = fn->reg_save_area->offset;
            MOV(INDIRECT(RBP, -offset), RDI);
            MOV(INDIRECT(RBP, -offset + 8), RSI);
            MOV(INDIRECT(RBP, -offset + 16), RDX);
            MOV(INDIRECT(RBP, -offset + 24), RCX);
            MOV(INDIRECT(RBP, -offset + 32), R8);
            MOV(INDIRECT(RBP, -offset + 40), R9);
            MOVAPS(INDIRECT(RBP, -offset + 48), XMM0);
            MOVAPS(INDIRECT(RBP, -offset + 64), XMM1);
            MOVAPS(INDIRECT(RBP, -offset + 80), XMM2);
            MOVAPS(INDIRECT(RBP, -offset + 96), XMM3);
            MOVAPS(INDIRECT(RBP, -offset + 112), XMM4);
            MOVAPS(INDIRECT(RBP, -offset + 128), XMM5);
            MOVAPS(INDIRECT(RBP, -offset + 144), XMM6);
            MOVAPS(INDIRECT(RBP, -offset + 160), XMM7);
        }

        // レジスタによって渡された引数の値をスタックに保存する
        int iparam = 0;
        int fparam = 0;
        for (Obj *var = fn->locals; iparam + fparam < fn->nparams;
             var = var->next) {
            if (is_flonum(var->ty)) {
                if (fparam < FP_MAX) {
                    var->ty->kind == TY_FLOAT
                        ? MOVSS(INDIRECT(RBP, -var->offset), argregf[fparam])
                        : MOVSD(INDIRECT(RBP, -var->offset), argregf[fparam]);
                }
                fparam++;
                continue;
            }

            if (iparam < GP_MAX) {
                switch (var->ty->size) {
                case 1:
                    MOV(INDIRECT(RBP, -var->offset), argreg8[iparam]);
                    break;
                case 2:
                    MOV(INDIRECT(RBP, -var->offset), argreg16[iparam]);
                    break;
                case 4:
                    MOV(INDIRECT(RBP, -var->offset), argreg32[iparam]);
                    break;
                case 8:
                    MOV(INDIRECT(RBP, -var->offset), argreg64[iparam]);
                    break;
                default:
                    unreachable();
                }
            }

            iparam++;
        }

        // 先頭の式から順にコード生成
        gen_stmt(fn->body);
        assert(depth == 0);

        // エピローグ
        println(".Lreturn.%s:", fn->name);
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
