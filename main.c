#include "9cc.h"

static char *input_path;
FILE *outfp;

static struct {
    char *o;
    bool E;
    Vector include;
} option;

void initialize_global_objects() {
    include_paths = vector_new();
    macros = hashmap_new();
    option.include = vector_new();
}

static void read_D_option(char *s) {
    char *eq = strchr(s, '=');
    if (eq) {
        define_macro(strndup(s, eq - s), eq + 1);
    } else {
        define_macro(s, "1");
    }
}

static void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            option.o = argv[++i];
            continue;
        }

        if (!strncmp(argv[i], "-o", 2)) {
            option.o = argv[i] + 2;
            continue;
        }

        if (!strcmp(argv[i], "-E")) {
            option.E = true;
            continue;
        }

        if (!strcmp(argv[i], "-I")) {
            vector_push(include_paths, argv[++i]);
            continue;
        }

        if (!strncmp(argv[i], "-I", 2)) {
            vector_push(include_paths, argv[i] + 2);
            continue;
        }

        if (!strcmp(argv[i], "-D")) {
            read_D_option(argv[++i]);
            continue;
        }

        if (!strncmp(argv[i], "-D", 2)) {
            read_D_option(argv[i] + 2);
            continue;
        }

        if (!strcmp(argv[i], "-U")) {
            i++;
            hashmap_delete(macros, argv[i], (int)strlen(argv[i]));
            continue;
        }

        if (!strncmp(argv[i], "-U", 2)) {
            hashmap_delete(macros, argv[i] + 2, (int)strlen(argv[i] + 2));
            continue;
        }

        if (!strcmp(argv[i], "-include")) {
            vector_push(option.include, argv[++i]);
            continue;
        }

        if (!strncmp(argv[i], "-include", 8)) {
            vector_push(option.include, argv[i] + 8);
            continue;
        }

        if (!strcmp(argv[i], "-hashmap-test")) {
            hashmap_test();
            exit(0);
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("無効な引数です: %s", argv[i]);
        }

        if (input_path) {
            error("複数のファイルを指定することはできません");
        }
        input_path = argv[i];
    }
}

Vector include_paths;

void add_default_include_paths(char *argv0) {
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

void print_tokens(Token *tok) {
    for (Token *t = tok; !at_eof(t); t = t->next) {
        if (t->at_bol && t != tok) {
            putc('\n', outfp);
        }
        if (t->has_space && !t->at_bol) {
            putc(' ', outfp);
        }
        fprintf(outfp, "%.*s", t->len, t->loc);
    }
    putc('\n', outfp);
}

static char *replace_extension(char *path, char *ext) {
    char *p = strrchr(path, '.');
    if (!p) {
        return format("%s.%s", path, ext);
    }

    *p = '\0';
    return format("%s.%s", path, ext);
}

static FILE *open_output_file(char *path) {
    if (strcmp(path, "-") == 0) {
        return stdout;
    }

    FILE *fp = fopen(path, "w");
    if (!fp) {
        error("ファイルを開くことができません(\"%s\"): %s", path,
              strerror(errno));
    }
    return fp;
}

int main(int argc, char **argv) {
    initialize_global_objects();

    parse_args(argc, argv);

    add_default_include_paths(argv[0]);

    // トークナイズ
    Token *token = NULL, *token2;
    for (int i = 0; i < vector_size(option.include); i++) {
        char *incl = vector_get(option.include, i);
        char *path;
        if (access(incl, R_OK) == 0) {
            path = incl;
        } else {
            path = search_include_paths(incl);
            if (!path) {
                error("ファイルが見つかりません: %s", incl);
            }
        }

        token2 = tokenize_file(path);
        token = append(token, token2);
    }

    token2 = tokenize_file(input_path);
    token = append(token, token2);

    // プリプロセス
    token = preprocess(token);
    if (option.E) {
        outfp = open_output_file(option.o ? option.o : "-");
        print_tokens(token);
        return 0;
    }

    // パース
    program(token);

    // コード生成
    outfp = open_output_file(option.o ? option.o
                                      : replace_extension(input_path, "s"));
    codegen();
    if (outfp != stdout) {
        fclose(outfp);
    }

    return 0;
}
