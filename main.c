#include "9cc.h"

//下をアンコメントしてデバッグモード
// #define ___DEBUG

char *user_input;

Token *token;

Node *code[100];

LVar *locals;

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    //トークナイズ&パース
    user_input = argv[1];
    token = tokenize();

#ifdef ___DEBUG
    for (Token *var = token; var->next; var = var->next) {
        printf("# debug:: token->str: %s\n", var->str);
        printf("# debug:: token->str: %d\n", var->kind);
    }
#endif

    program();

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //プロローグ
    //変数26個分の領域(8 * 26 = 208)を確保する
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n");

    //先頭の式から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        //スタックトップに式全体の値が残っているはずなのでスタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }

    //エピローグ
    //最後の式の結果がRAXに残っているのでそれが戻り値となる
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return 0;
}