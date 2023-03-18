#include "9cc.h"

//
// コード生成部
//

static int label_count;
static char *const argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char *const argreg32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static char *const argreg16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
static char *const argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

int count() {
    static int i = 1;
    return i++;
}

// nをalignの直近の倍数に切り上げる
int align_to(int n, int align) {
    assert(align != 0);
    return (n + align - 1) / align * align;
}

static void load(Type *ty) {
    if (is_type_of(TY_ARY, ty) || is_type_of(TY_STRUCT, ty)) {
        return;
    }

    printf("    pop rax\n");

    switch (ty->size) {
    case 1:
        printf("    movsx eax, BYTE PTR [rax]\n");
        break;
    case 2:
        printf("    movsx eax, WORD PTR [rax]\n");
        break;
    case 4:
        printf("    mov eax, DWORD PTR [rax]\n");
        break;
    case 8:
        printf("    mov rax, [rax]\n");
        break;
    default:;
    }

    printf("    push rax\n");
}

static void store(Type *ty) {
    printf("    pop rdi\n");
    printf("    pop rax\n");

    if (is_type_of(TY_STRUCT, ty)) {
        for (Member *mem = ty->members; mem; mem = mem->next) {
            switch (mem->ty->size) {
            case 1:
                printf("    movsx edx, BYTE PTR [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], dl\n", mem->offset);
                continue;
            case 2:
                printf("    movsx edx, WORD PTR [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], dx\n", mem->offset);
                continue;
            case 4:
                printf("    mov edx, DWORD PTR [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], edx\n", mem->offset);
                continue;
            case 8:
                printf("    mov rdx, [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], rdx\n", mem->offset);
                continue;
            default:;
            }
        }
    } else {
        switch (ty->size) {
        case 1:
            printf("    mov [rax], dil\n");
            break;
        case 2:
            printf("    mov [rax], di\n");
            break;
        case 4:
            printf("    mov [rax], edi\n");
            break;
        case 8:
            printf("    mov [rax], rdi\n");
            break;
        default:;
        }
    }

    printf("    push rdi\n");
}

static void gen_lval(Node *node) {
    switch (node->kind) {
    case ND_VAR:
        if (node->var->is_lvar) {
            // ローカル変数
            printf("    mov rax, rbp\n");
            printf("    sub rax, %d\n", node->var->offset);
            printf("    push rax\n");
        } else {
            // グローバル変数
            printf("    lea rax, %s[rip]\n", node->var->name);
            printf("    push rax\n");
        }
        return;
    case ND_MEMBER:
        gen_lval(node->lhs);
        printf("    pop rax\n");
        printf("    add rax, %d\n", node->member->offset);
        printf("    push rax\n");
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    default:;
    }

    error_tok(node->tok, "代入の左辺値が変数ではありません");
}

static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        printf("    push %ld\n", node->val);
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_lval(node);
        load(node->ty);
        return;
    case ND_ADDR:
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        load(node->ty);
        return;
    case ND_FUNCALL: {
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            gen_expr(arg);
            nargs++;
        }

        for (int i = nargs - 1; i >= 0; i--) {
            printf("    pop %s\n", argreg64[i]);
        }

        printf("    mov rax, 0\n");
        printf("    call %s\n", node->funcname);
        printf("    push rax\n");
        return;
    }
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen_expr(node->rhs);
        store(node->ty);
        return;
    case ND_NOT:
        gen_expr(node->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        printf("    push rax\n");
        return;
    case ND_COMMA:
        gen_expr(node->lhs);
        printf("    pop rax\n");
        gen_expr(node->rhs);
        return;
    case ND_STMT_EXPR:
        for (Node *n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        printf("    push rax\n");
        return;
    case ND_NULL_EXPR:
        printf("    push rax\n");
        return;
    default:;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    char *ax = node->lhs->ty->kind == TY_LONG || node->lhs->ty->base ? "rax" : "eax";
    char *di = node->lhs->ty->kind == TY_LONG || node->lhs->ty->base ? "rdi" : "edi";

    switch (node->kind) {
    case ND_ADD:
        printf("    add %s, %s\n", ax, di);
        break;
    case ND_SUB:
        printf("    sub %s, %s\n", ax, di);
        break;
    case ND_MUL:
        printf("    imul %s, %s\n", ax, di);
        break;
    case ND_DIV:
        if (node->lhs->ty->size == 8) {
            printf("    cqo\n");
        } else {
            printf("    cdq\n");
        }
        printf("    idiv %s\n", di);
        break;
    case ND_EQ:
        printf("    cmp %s, %s\n", ax, di);
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp %s, %s\n", ax, di);
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp %s, %s\n", ax, di);
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp %s, %s\n", ax, di);
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LOGAND: {
        int c = count();
        gen_expr(node->lhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lfalse%d\n", c);
        gen_expr(node->rhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lfalse%d\n", c);
        printf("    mov rax, 1\n");
        printf("    jmp .Lend%d\n", c);
        printf(".Lfalse%d:\n", c);
        printf("    mov rax, 0\n");
        printf(".Lend%d:\n", c);
        break;
    }
    case ND_LOGOR: {
        int c = count();
        gen_expr(node->lhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    jne .Ltrue%d\n", c);
        gen_expr(node->rhs);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    jne .Ltrue%d\n", c);
        printf("    mov rax, 0\n");
        printf("    jmp .Lend%d\n", c);
        printf(".Ltrue%d:\n", c);
        printf("    mov rax, 1\n");
        printf(".Lend%d:\n", c);
        break;
    }
    default:;
    }

    printf("    push rax\n");
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
    case ND_BLOCK: {
        for (Node *cur = node->body; cur; cur = cur->next) {
            gen_stmt(cur);
        }

        return;
    }
    case ND_GOTO:
        printf("    jmp .L%d\n", node->label_id);
        return;
    case ND_LABEL:
        printf(".L%d:\n", node->label_id);
        gen_stmt(node->lhs);
        return;
    case ND_RETURN:
        gen_expr(node->lhs);

        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_IF: {
        int c = count();
        gen_expr(node->cond);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lelse%d\n", c);

        gen_stmt(node->then);

        printf("    jmp .Lend%d\n", c);
        printf(".Lelse%d:\n", c);

        if (node->els) {
            gen_stmt(node->els);
        }

        printf(".Lend%d:\n", c);

        return;
    }
    case ND_SWITCH:
        gen_expr(node->cond);
        printf("    pop rax\n");

        for (Node *n = node->cases; n; n = n->case_next) {
            printf("    cmp rax, %ld\n", n->case_val);
            printf("    je .L%d\n", n->label_id);
        }

        if (node->default_case) {
            printf("    jmp .L%d\n", node->default_case->label_id);
        }

        printf("    jmp .L%d\n", node->brk_label_id);
        gen_stmt(node->then);
        printf(".L%d:\n", node->brk_label_id);
        return;
    case ND_CASE:
        printf(".L%d:\n", node->label_id);
        gen_stmt(node->lhs);
        return;
    case ND_LOOP: {
        int c = count();

        if (node->init) {
            gen_stmt(node->init);
        }

        printf(".Lbegin%d:\n", c);

        if (node->cond) {
            gen_expr(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", node->brk_label_id);
        }

        gen_stmt(node->then);
        printf(".L%d:\n", node->cont_label_id);
        if (node->after) {
            gen_expr(node->after);
            printf("    pop rax\n");
        }

        printf("    jmp .Lbegin%d\n", c);
        printf(".L%d:\n", node->brk_label_id);
        return;
    }
    case ND_INIT:
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        printf("    pop rax\n");
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
            printf("    .zero %d\n", padding);
        }
    }

    if (cur->expr) {
        int64_t val = calc_const_expr(cur->expr);
        switch (cur->ty->size) {
        case 1:
            printf("    .byte %ld\n", val);
            return;
        case 2:
            printf("    .value %ld\n", val);
            return;
        case 4:
            printf("    .long %ld\n", val);
            return;
        case 8:
            printf("    .quad %ld\n", val);
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
            emit_gvar_init(cur->children[mem->next->idx], cur->children[mem->idx]);
        }

        int padding = cur->ty->align - mem->ty->align;
        if (padding > 0) {
            printf("    .zero %d\n", padding);
        }
        break;
    }
    default:;
    }
}

static void emit_global_variables() {
    if (!globals) {
        return;
    }

    for (Var *var = globals; var; var = var->next) {
        printf("    .globl %s\n", var->name);
        if (var->init_data_str) {
            printf("    .section .rodata\n");
            printf("%s:\n", var->name);
            printf("    .string \"%s\"\n", var->init_data_str);
        } else if (var->init) {
            printf("    .data\n");
            printf("%s:\n", var->name);
            emit_gvar_init(var->init, NULL);
        } else {
            printf("    .bss\n");
            printf("%s:\n", var->name);
            printf("    .zero %d\n", var->ty->size);
        }
    }
}

static void emit_functions() {
    for (Function *fn = functions; fn; fn = fn->next) {
        if (!fn->has_definition) {
            continue;
        }

        if (locals) {
            fn->stack_size = align_to(locals->offset, 16);
        }

        // アセンブリの前半部分を出力
        printf("    .globl %s\n", fn->name);
        printf("    .text\n");
        printf("%s:\n", fn->name);

        // プロローグ
        // 変数の領域を確保する
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", fn->stack_size);

        int nparams = 0;
        for (Var *var = fn->params; var; var = var->next) {
            nparams++;
        }

        // レジスタによって渡された引数の値をスタックに保存する
        int i = nparams - 1;
        for (Var *var = fn->params; var; var = var->next) {
            switch (var->ty->size) {
            case 1:
                printf("    mov [rbp-%d], %s\n", var->offset, argreg8[i--]);
                continue;
            case 2:
                printf("    mov [rbp-%d], %s\n", var->offset, argreg16[i--]);
                continue;
            case 4:
                printf("    mov [rbp-%d], %s\n", var->offset, argreg32[i--]);
                continue;
            case 8:
                printf("    mov [rbp-%d], %s\n", var->offset, argreg64[i--]);
                continue;
            default:;
            }
        }

        // 先頭の式から順にコード生成
        gen_stmt(fn->body);

        // スタックトップに式全体の値が残っているはずなのでスタックが溢れないようにポップしておく
        printf("    pop rax\n");

        // エピローグ
        // 最後の式の結果がRAXに残っているのでそれが戻り値となる
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
    }
}

void codegen() {
    printf("    .intel_syntax noprefix\n");

    emit_global_variables();
    emit_functions();
}
