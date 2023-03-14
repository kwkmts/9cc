#include "9cc.h"

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

void error_tok(Token *tok, char *fmt, ...) {
    error_at(tok->str, fmt);
}

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

static bool is_ident1(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_ident2(char c) { return is_ident1(c) || ('0' <= c && c <= '9'); }

static bool is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || (c == '_');
}

static int read_keyword(char *c) {
    static char *kw[] = {"if", "else", "while", "for", "goto", "break", "return",
                         "void", "int", "char", "short", "long", "struct", "sizeof"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        int len = strlen(kw[i]);
        if (strncmp(c, kw[i], len) == 0 && !is_alnum(c[len])) {
            return len;
        }
    }
    return 0;
}

static bool is_punct_with_2char(char *p) {
    static char *kw[] = {"==", "!=", "<=", ">=", "+=", "-=", "*=", "/=",
                         "++", "--", "&&", "||", "->"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (startswith(p, kw[i])) {
            return true;
        }
    }
    return false;
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

static char *read_escaped_char(const char *p) {
    switch (*p) {
    case 'a':
        return "\\007";
    case 'b':
        return "\\b";
    case 't':
        return "\\t";
    case 'n':
        return "\\n";
    case 'v':
        return "\\013";
    case 'f':
        return "\\f";
    case 'r':
        return "\\r";
    case 'e':
        return "\\033";// GNU拡張(ASCII ESC)
    case '"':
        return "\\\"";
    case '\\':
        return "\\\\";
    case '0':
        return "\\000";
    default:
        return strndup(p, 1);
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

        // 2文字の区切り文字
        if (is_punct_with_2char(p)) {
            cur = cur->next = new_token(TK_RESERVED, p, 2);
            p += 2;
            continue;
        }

        // 1文字の区切り文字
        if (strchr("+-*/&()<>{}[]=!:;,.", *p)) {
            cur = cur->next = new_token(TK_RESERVED, p++, 1);
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
                    strcat(buf, read_escaped_char(p + 1));
                    len++;
                    p += 2;
                    continue;
                }

                strcat(buf, strndup(p, 1));
                len++;
                p++;
            }

            cur = cur->next = new_token(TK_STR, start, len);
            cur->str = buf;

            p++;//結びの`"`を読み飛ばす
            continue;
        }

        // 識別子
        if (is_ident1(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_ident2(*p));

            cur = cur->next = new_token(TK_IDENT, start, p - start);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    cur->next = new_token(TK_EOF, p, 0);
    return head.next;
}
