#include "9cc.h"

char *filepath;
char *user_input;
Token *token;
Function prog;
Var *locals;
Var *globals;
Type *ty_char = &(Type){TY_CHAR, 1};
Type *ty_int = &(Type){TY_INT, 8};

static char *read_file(char *path) {
    FILE *fp;
    if (strcmp(path, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if (fp == NULL) {
            error("ファイルを開くことができません(\"%s\"): %s", path, strerror(errno));
        }
    }

    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    for (;;) {
        char buf[4096];
        int n = fread(buf, 1, sizeof(buf), fp);
        if (n == 0) {
            break;
        }
        fwrite(buf, 1, n, out);
    }

    if (fp != stdin) {
        fclose(fp);
    }

    fflush(out);
    if (buflen == 0 || buf[buflen - 1] != '\n') {
        fputc('\n', out);
    }
    fputc('\0', out);
    fclose(out);

    return buf;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
    }

    // トークナイズ
    user_input = read_file(filepath = argv[1]);
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
