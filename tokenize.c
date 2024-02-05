#include "9cc.h"

static File *cur_file;
File **input_files;

static bool at_bol; // 行頭かどうか
static bool has_space;

// エラーを報告する関数
//  printf()と同じ引数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告
void error_at(File *file, char *loc, char *fmt, ...) {
    char *msg;
    size_t buflen;
    FILE *out = open_memstream(&msg, &buflen);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fclose(out);

    char *line = loc;
    while (line > file->content && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    int line_no = 1;
    for (char *p = file->content; p < line; p++) {
        if (*p == '\n') {
            line_no++;
        }
    }

    int column_no = (int)(loc - line + 1);

    int indent = fprintf(stderr, "%s:%d:%d: ", file->name, line_no, column_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = (int)(loc - line + indent);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    fprintf(stderr, "%s\n", msg);

    FILE *fp = fopen("/tmp/9cc_err_info", "w");
    if (fp) {
        fprintf(fp, "%d\n", line_no);
        fprintf(fp, "%d\n", column_no);
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }

    exit(1);
}

void error_tok(Token *tok, char *fmt, ...) {
    char *msg;
    size_t buflen;
    FILE *out = open_memstream(&msg, &buflen);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fclose(out);
    error_at(tok->file, tok->loc, msg);
}

//
// トークナイザー
//

static Token *new_token(TokenKind kind, char *loc, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = loc;
    tok->len = len;
    tok->file = cur_file;
    tok->at_bol = at_bol;
    tok->has_space = has_space;
    at_bol = has_space = false;
    return tok;
}

Token *copy_token(Token *tok) {
    Token *ret = calloc(1, sizeof(Token));
    *ret = *tok;
    return ret;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

static bool is_keyword(Token *tok) {
    static char *kw[] = {
        "if",       "else",
        "switch",   "case",
        "default",  "do",
        "while",    "for",
        "goto",     "break",
        "continue", "return",
        "unsigned", "signed",
        "void",     "int",
        "char",     "short",
        "long",     "_Bool",
        "float",    "double",
        "struct",   "union",
        "enum",     "__builtin_va_list",
        "sizeof",   "typedef",
        "static",   "extern",
        "const",    "volatile",
        "restrict",
    };
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = (int)strlen(kw[i]);
        if (strncmp(tok->loc, kw[i], len) == 0 && !is_alnum(tok->loc[len])) {
            return true;
        }
    }
    return false;
}

void convert_keywords(Token *tok) {
    for (Token *t = tok; !at_eof(t); t = t->next) {
        if (is_keyword(t)) {
            t->kind = TK_KEYWORD;
        }
    }
}

static int read_punct(char *p) {
    static char *kw[] = {"<<=", ">>=", "==", "!=", "<=", ">=",  "<<", ">>",
                         "+=",  "-=",  "*=", "/=", "%=", "&=",  "|=", "^=",
                         "++",  "--",  "&&", "||", "->", "...", "##"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (startswith(p, kw[i])) {
            return (int)strlen(kw[i]);
        }
    }
    return strchr("+-*/%&|()<>{}[]=~^!?:;,.#`", *p) ? 1 : 0;
}

static char *str_literal_end(char *p) {
    while (*p != '"') {
        if (*p == '\n' || *p == '\0') {
            error_at(cur_file, p, "'\"'がありません");
        }
        if (*p == '\\') {
            p++;
        }
        p++;
    }
    return p;
}

static char read_char(char **p) {
    if ((*p)[0] != '\\') {
        return ((*p)++)[0];
    }

    char c = (*p)[1];
    (*p) += 2;

    switch (c) {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 't':
        return '\t';
    case 'n':
        return '\n';
    case 'v':
        return '\v';
    case 'f':
        return '\f';
    case 'r':
        return '\r';
    case 'e':
        return 27; // GNU拡張(ASCII ESC)
    case '"':
        return '\"';
    case '\\':
        return '\\';
    case '0':
        return '\0';
    default:
        return c;
    }
}

static Type *read_int_suffix(char **p, int64_t val, int base) {
    char *start = *p;
    bool is_long = false;
    bool is_unsigned = false;

    for (;;) {
        if (!is_long) {
            if (strncmp(*p, "LL", 2) == 0 || strncmp(*p, "ll", 2) == 0) {
                (*p) += 2;
                is_long = true;
                continue;
            } else if (strncasecmp(*p, "L", 1) == 0) {
                (*p)++;
                is_long = true;
                continue;
            }
        }

        if (!is_unsigned && strncasecmp(*p, "U", 1) == 0) {
            (*p)++;
            is_unsigned = true;
            continue;
        }

        if (is_alnum(**p)) {
            error_at(cur_file, start, "不正なサフィックスです");
        }

        break;
    }

    if (base == 10) {
        if (is_long && is_unsigned) {
            return ty_ulong;
        } else if (is_long) {
            return ty_long;
        } else if (is_unsigned) {
            return (val >> 32) ? ty_ulong : ty_uint;
        } else {
            return (val >> 31) ? ty_long : ty_int;
        }
    } else {
        if (is_long && is_unsigned || val >> 63) {
            return ty_ulong;
        } else if (is_long) {
            return (val >> 63) ? ty_ulong : ty_long;
        } else if (is_unsigned) {
            return (val >> 32) ? ty_ulong : ty_uint;
        } else if (val >> 32) {
            return ty_long;
        } else if (val >> 31) {
            return ty_uint;
        } else {
            return ty_int;
        }
    }
}

static Type *read_float_suffix(char **p) {
    char *start = *p;
    Type *ty;
    if (strncasecmp(*p, "f", 1) == 0) {
        (*p)++;
        ty = ty_float;
    } else if (strncasecmp(*p, "l", 1) == 0) {
        (*p)++;
        ty = ty_double;
    } else {
        ty = ty_double;
    }

    if (is_alnum(**p)) {
        error_at(cur_file, start, "不正なサフィックスです");
    }

    return ty;
}

static void read_num_literal(char **p, Token **tok) {
    int base;
    if (strncasecmp(*p, "0x", 2) == 0 && is_alnum((*p)[2])) {
        *p += 2;
        base = 16;
    } else if (**p == '0') {
        base = 8;
    } else {
        base = 10;
    }

    char *start = *p;
    int64_t i = (int64_t)strtoul(*p, p, base);
    if (**p == '.') {
        // ピリオドを読んだなら数値リテラルの最初の'0'は8進数の'0'ではなく
        // 浮動小数点数リテラルの一部としての'0'
        if (base == 8) {
            base = 10;
        }

        if (base != 10) {
            error_at(cur_file, start,
                     "浮動小数点数で10進数以外の表記はできません");
        }

        *p = start;
        (*tok)->fval = strtod(*p, p);
        if (**p == '.') {
            error_at(cur_file, *p, "小数点が多すぎます");
        }

        (*tok)->val_ty = read_float_suffix(p);
    } else {
        (*tok)->ival = i;
        (*tok)->val_ty = read_int_suffix(p, (*tok)->ival, base);
    }
}

static void add_line_column_no(Token *tok) {
    int line_no = 1, column_no = 1;
    for (char *p = tok->file->content; *p; p++) {
        if (p == tok->loc) {
            tok->line_no = line_no;
            tok->column_no = column_no;
            tok = tok->next;
        }

        if (*p == '\n') {
            line_no++;
            column_no = 1;
        } else {
            column_no++;
        }
    }
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    at_bol = true;
    has_space = false;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            has_space = true;
            if (*p == '\n') {
                at_bol = true;
                has_space = false;
            }
            p++;
            continue;
        }

        // 行コメントをスキップ
        if (startswith(p, "//")) {
            has_space = true;
            p += 2;
            while (*p != '\n') {
                p++;
            }
            continue;
        }

        // ブロックコメントをスキップ
        if (startswith(p, "/*")) {
            has_space = true;
            char *q = strstr(p + 2, "*/");
            if (q == NULL) {
                error_at(cur_file, p, "コメントが閉じられていません");
            }
            p = q + 2;
            continue;
        }

        // 数値リテラル
        if (isdigit(*p) || (*p == '.' && isdigit(p[1]))) {
            char *start = p;
            cur = cur->next = new_token(TK_NUM, p, 0);
            read_num_literal(&p, &cur);
            cur->len = (int)(p - start);
            continue;
        }

        // 区切り文字
        int length = read_punct(p);
        if (length) {
            cur = cur->next = new_token(TK_RESERVED, p, length);
            p += length;
            continue;
        }

        // 文字リテラル
        if (*p == '\'') {
            char *start = p;
            cur = cur->next = new_token(TK_NUM, p++, 0);
            cur->ival = (int64_t)read_char(&p);
            cur->val_ty = ty_int;
            cur->len = (int)(p - start + 1);
            if (*p != '\'') {
                error_at(cur_file, p, "'''ではありません");
            }
            p++;
            continue;
        }

        // 文字列リテラル
        if (*p == '"') {
            char *start = p++;
            char *end = str_literal_end(start + 1);
            char *buf = calloc(end - start, sizeof(char));
            for (int i = 0; *p != '"'; i++) {
                buf[i] = read_char(&p);
            }

            cur = cur->next = new_token(TK_STR, start, (int)(end - start + 1));
            cur->str = buf;
            p++; // 結びの`"`を読み飛ばす
            continue;
        }

        // 識別子
        if (is_alpha(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_alnum(*p));

            cur = cur->next = new_token(TK_IDENT, start, (int)(p - start));
            continue;
        }

        error_at(cur_file, p, "トークナイズできません");
    }

    cur->next = new_token(TK_EOF, p, 0);

    add_line_column_no(head.next);
    return head.next;
}

static File *new_file(char *name, char *content, int number) {
    File *file = calloc(1, sizeof(File));
    file->name = name;
    file->content = content;
    file->number = number;
    return file;
}

Token *tokenize_file(char *path) {
    char *p = read_file(path);

    static int input_file_count;

    File *file = new_file(path, p, input_file_count + 1);

    input_files = realloc(input_files, sizeof(File *) * (input_file_count + 2));
    input_files[input_file_count] = file;
    input_files[input_file_count + 1] = NULL;
    input_file_count++;

    cur_file = file;
    return tokenize(file->content);
}
