#include "9cc.h"

#ifndef ___DEBUG
// 下の１行をアンコメントしてデバッグフラグを有効化
//  #define ___DEBUG
#endif

// グローバル変数の定義
char *user_input;
Token *token;
Function prog;
LVar *locals;
Type *ty_int = &(Type){TY_INT, 8};

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    // トークナイズ
    user_input = argv[1];
    token = tokenize();

#ifdef ___DEBUG
    for (Token *var = token; var->next; var = var->next) {
        printf("# debug:: [%d]token->str: %s\n", var->kind, var->str);
    }
#endif

    // パース
    program();

    // コード生成
    codegen();

    return 0;
}
