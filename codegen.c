#include "9cc.h"

#ifndef ___DEBUG
//下の１行をアンコメントしてデバッグフラグを有効化
// #define ___DEBUG
#endif

//
//コード生成部
//

static int label_count;

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

static void gen_expr(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            printf("    push %d\n", node->val);
            return;
        case ND_LVAR:
            gen_lval(node);

            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            return;
        case ND_FUNCALL:
            printf("    mov rax, 0\n");
            printf("    call %s\n", node->funcname);
            printf("    push rax\n");
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            gen_expr(node->rhs);

            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
            printf("    push rdi\n");
            return;
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
    }

    printf("    push rax\n");
}

static void gen_stmt(Node *node) {
    switch (node->kind) {
        case ND_BLOCK: {
            for (Node *cur = node->body; cur; cur = cur->next) gen_stmt(cur);

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

            if (node->els) gen_stmt(node->els);

            printf(".Lend%d:\n", c);

            return;
        }
        case ND_LOOP: {
            int c = label_count++;

            if (node->init) gen_expr(node->init);

            printf(".Lbegin%d:\n", c);

            if (node->cond) gen_expr(node->cond);

            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je .Lend%d\n", c);

            gen_stmt(node->then);
            if (node->after) gen_expr(node->after);

            printf("    jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            return;
        }
        case ND_NULL_STMT:
            return;
        default:
            gen_expr(node);
    }
}

void codegen() {
    int offset = 0;
    for (LVar *var = locals; var; var = var->next) {
        offset += 8;
        var->offset = -offset;
    }

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //プロローグ
    //変数の領域を確保する
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", offset);

    //先頭の式から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen_stmt(code[i]);

        //スタックトップに式全体の値が残っているはずなのでスタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }

    //エピローグ
    //最後の式の結果がRAXに残っているのでそれが戻り値となる
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
}
