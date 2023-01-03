#include "9cc.h"

#ifndef ___DEBUG
// 下の１行をアンコメントしてデバッグフラグを有効化
//  #define ___DEBUG
#endif

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
    fprintf(stderr, "%.*s\n", end - line, line);

    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//
// トークナイザー
//

// 新しいトークンを作成してcurに繋げる
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

static bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

static bool is_ident1(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_ident2(char c) { return is_ident1(c) || ('0' <= c && c <= '9'); }

static bool is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || (c == '_');
}

static size_t is_keyword(char *c) {
    char *kw[] = {"if", "else", "while", "for", "return", "int",
                  "char", "sizeof"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        size_t len = strlen(kw[i]);
        if (strncmp(c, kw[i], len) == 0 && !is_alnum(c[len])) {
            return len;
        }
    }
    return 0;
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
        size_t length = is_keyword(p);
        if (length) {
            cur = new_token(TK_KEYWORD, cur, p, length);
            p += length;
            continue;
        }

        // 2文字の区切り文字
        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") ||
            startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        // 1文字の区切り文字
        if (strchr("+-*/&()<>{}[]=;,", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // 数値リテラル
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        // 文字列リテラル
        if (*p == '"') {
            char *start = ++p;
            int len = 0;
            while (*p != '"') {
                len++;
                p++;
                if (*p == '\n' || *p == '\0') {
                    error_at(p, "'\"'がありません");
                }
            }
            cur = new_token(TK_STR, cur, start, len);
            p++;//結びの`"`を読み飛ばす
            continue;
        }

        // 識別子
        if (is_ident1(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_ident2(*p));

            cur = new_token(TK_IDENT, cur, start, p - start);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}
