#include "9cc.h"

char *user_input;

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
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *line = loc;
    while (line > user_input && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    int indent = fprintf(stderr, "%s:%d: ", filepath, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    int pos = (int)(loc - line + indent);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_tok(Token *tok, char *fmt, ...) { error_at(tok->str, fmt); }

//
// トークナイザー
//

static Token *new_token(TokenKind kind, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

static int read_keyword(char *c) {
    static char *kw[] = {"if",     "else",   "switch", "case",  "default",
                         "while",  "for",    "goto",   "break", "continue",
                         "return", "void",   "int",    "char",  "short",
                         "long",   "struct", "union",  "sizeof"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = (int)strlen(kw[i]);
        if (strncmp(c, kw[i], len) == 0 && !is_alnum(c[len])) {
            return len;
        }
    }
    return 0;
}

static int read_punct(char *p) {
    static char *kw[] = {"<<=", ">>=", "==", "!=", "<=", ">=", "<<",
                         ">>",  "+=",  "-=", "*=", "/=", "%=", "&=",
                         "|=",  "^=",  "++", "--", "&&", "||", "->"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (startswith(p, kw[i])) {
            return (int)strlen(kw[i]);
        }
    }
    return strchr("+-*/%&|()<>{}[]=~^!?:;,.", *p) ? 1 : 0;
}

static char *str_literal_end(char *p) {
    while (*p != '\"') {
        if (*p == '\n' || *p == '\0') {
            error_at(p, "'\"'がありません");
        }
        p++;
    }
    return p;
}

static void read_escaped_char(char *buf, const char *p) {
    switch (*p) {
    case 'a':
        strcat(buf, "\\007");
        return;
    case 'b':
        strcat(buf, "\\b");
        return;
    case 't':
        strcat(buf, "\\t");
        return;
    case 'n':
        strcat(buf, "\\n");
        return;
    case 'v':
        strcat(buf, "\\013");
        return;
    case 'f':
        strcat(buf, "\\f");
        return;
    case 'r':
        strcat(buf, "\\r");
        return;
    case 'e':
        strcat(buf, "\\033"); // GNU拡張(ASCII ESC)
        return;
    case '"':
        strcat(buf, "\\\"");
        return;
    case '\\':
        strcat(buf, "\\\\");
        return;
    case '0':
        strcat(buf, "\\000");
        return;
    default:
        strncat(buf, p, 1);
        return;
    }
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
    char *p = user_input;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 行コメントをスキップ
        if (startswith(p, "//")) {
            p += 2;
            while (*p != '\n') {
                p++;
            }
            continue;
        }

        // ブロックコメントをスキップ
        if (startswith(p, "/*")) {
            char *q = strstr(p + 2, "*/");
            if (q == NULL) {
                error_at(p, "コメントが閉じられていません");
            }
            p = q + 2;
            continue;
        }

        // 予約語
        int length = read_keyword(p);
        if (length) {
            cur = cur->next = new_token(TK_KEYWORD, p, length);
            p += length;
            continue;
        }

        // 区切り文字
        length = read_punct(p);
        if (length) {
            cur = cur->next = new_token(TK_RESERVED, p, length);
            p += length;
            continue;
        }

        // 数値リテラル
        if (isdigit(*p)) {
            cur = cur->next = new_token(TK_NUM, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        // 文字列リテラル
        if (*p == '"') {
            char *start = ++p;
            char *end = str_literal_end(start);
            char *buf = calloc(2 * (end - start) + 1, sizeof(char));
            int len = 0;
            while (*p != '"') {
                if (*p == '\\') {
                    read_escaped_char(buf, p + 1);
                    len++;
                    p += 2;
                    continue;
                }

                strncat(buf, p, 1);
                len++;
                p++;
            }

            cur = cur->next = new_token(TK_STR, start, len);
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

        error_at(p, "トークナイズできません");
    }

    cur->next = new_token(TK_EOF, p, 0);
    return head.next;
}
