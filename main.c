#include "9cc.h"

//
// 動的配列の実装
//

struct Vector {
    void **data;
    int capacity;
    int size;
};

Vector vector_new() {
    Vector vec = calloc(1, sizeof(struct Vector));
    vec->capacity = 8;
    vec->data = calloc(vec->capacity, sizeof(void *));
    return vec;
}

void vector_push(Vector vec, void *val) {
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->size++] = val;
}

void *vector_get(Vector vec, int idx) { return vec->data[idx]; }

int vector_size(Vector vec) { return vec->size; }

Vector include_paths;

void add_default_include_paths(char *argv0) {
    include_paths = vector_new();
    vector_push(include_paths, format("%s/include", dirname(strdup(argv0))));
    vector_push(include_paths, "/usr/local/include");
    vector_push(include_paths, "/usr/include/x86_64-linux-gnu");
    vector_push(include_paths, "/usr/include");
}

char *read_file(char *path) {
    FILE *fp;
    if (strcmp(path, "-") == 0) {
        fp = stdin;
    } else {
        fp = fopen(path, "r");
        if (fp == NULL) {
            error("ファイルを開くことができません(\"%s\"): %s", path,
                  strerror(errno));
        }
    }

    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    for (;;) {
        char buf2[4096];
        size_t n = fread(buf2, 1, sizeof(buf2), fp);
        if (n == 0) {
            break;
        }
        fwrite(buf2, 1, n, out);
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

    char *filepath = argv[1];
    add_default_include_paths(argv[0]);

    // トークナイズ
    Token *token = tokenize_file(filepath);

    // プリプロセス
    token = preprocess(token);

    // パース
    program(token);

    // コード生成
    codegen();

    return 0;
}
