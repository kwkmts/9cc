#include "9cc.h"

char *user_input;

Token *token;

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    //トークナイズ&パース
    user_input = argv[1];
    token = tokenize();
    Node *node = expr();

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);

    //スタックトップに式全体の値が残っているはずなのでそれをRAXにロードして関数からの戻り値とする
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}