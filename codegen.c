#include "9cc.h"

//
// コード生成部
//

static int label_count;
static char *const argreg64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char *const argreg8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

// nをalignの直近の倍数に切り上げる
int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void load(Type *ty) {
    switch (ty->size) {
    case 1:
        printf("    movsx rax, BYTE PTR [rax]\n");
        return;
    case 8:
        printf("    mov rax, [rax]\n");
        return;
    default:;
    }
}

static void store(Type *ty) {
    if (is_type_of(TY_STRUCT, ty)) {
        for (Member *mem = ty->members; mem; mem = mem->next) {
            switch (mem->ty->size) {
            case 1:
                printf("    movsx rdx, BYTE PTR [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], dl\n", mem->offset);
                continue;
            case 8:
                printf("    mov rdx, [rdi+%d]\n", mem->offset);
                printf("    mov [rax+%d], rdx\n", mem->offset);
                continue;
            default:;
            }
        }
        return;
    }

    switch (ty->size) {
    case 1:
        printf("    mov [rax], dil\n");
        return;
    case 8:
        printf("    mov [rax], rdi\n");
        return;
    default:;
    }
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
        printf("    push %d\n", node->val);
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_lval(node);

        if (is_type_of(TY_ARY, node->ty) || is_type_of(TY_STRUCT, node->ty)) {
            return;
        }

        printf("    pop rax\n");
        load(node->ty);
        printf("    push rax\n");
        return;
    case ND_ADDR:
        gen_lval(node->lhs);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);

        printf("    pop rax\n");
        load(node->ty);
        printf("    push rax\n");
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

        printf("    pop rdi\n");
        printf("    pop rax\n");
        store(node->ty);
        printf("    push rdi\n");
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

    switch (node->kind) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LT:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LOGAND: {
        int c = label_count++;
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
        int c = label_count++;
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
    case ND_RETURN:
        gen_expr(node->lhs);

        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_IF: {
        int c = label_count++;
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
    case ND_LOOP: {
        int c = label_count++;

        if (node->init) {
            gen_expr(node->init);
            printf("    pop rax\n");
        }

        printf(".Lbegin%d:\n", c);

        if (node->cond) {
            gen_expr(node->cond);
        }

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", c);

        gen_stmt(node->then);
        if (node->after) {
            gen_expr(node->after);
            printf("    pop rax\n");
        }

        printf("    jmp .Lbegin%d\n", c);
        printf(".Lend%d:\n", c);
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

static void assign_lvar_offset(Function *fn) {
    if (locals != NULL) {
        fn->stack_size = align_to(locals->offset, 16);
    }
}

static void emit_gvar_init(Initializer *init) {
    if (init->expr) {
        int val = calc_const_expr(init->expr);
        switch (init->ty->size) {
        case 1:
            printf("    .byte %d\n", val);
        case 8:
            printf("    .quad %d\n", val);
        }

        return;
    }

    for (int i = 0; i < init->ty->ary_len; i++) {
        emit_gvar_init(init->children[i]);
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
            emit_gvar_init(var->init);
        } else {
            printf("    .bss\n");
            printf("%s:\n", var->name);
            printf("    .zero %d\n", var->ty->size);
        }
    }
}

static void emit_functions() {
    for (Function *fn = prog.next; fn; fn = fn->next) {
        assign_lvar_offset(fn);

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
