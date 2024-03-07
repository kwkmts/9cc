#include "9cc.h"

//
// プリプロセッサ
//

static Token *token; // 現在着目しているトークン
static Token *eof_token = &(Token){TK_EOF};
Hashmap macros;

typedef struct CondIncl CondIncl;
struct CondIncl {
    CondIncl *parent;
    enum { IN_THEN, IN_ELIF, IN_ELSE } ctx;
    Token *tok;
    bool included;
};

static CondIncl *cond_incl;

typedef struct MacroParam MacroParam;

typedef struct {
    enum { OBJLIKE, FUNCLIKE } kind;
    Token *body;
    MacroParam *params;
} Macro;

struct MacroParam {
    Token *name;
    MacroParam *next;
};

static Token *consume(char *op, TokenKind kind) {
    if (!equal(op, kind, token)) {
        return false;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

static bool consume_hash() {
    if (!equal("#", TK_RESERVED, token) || !token->at_bol) {
        return false;
    }
    token = token->next;
    return true;
}

static void expect(char *op) {
    if (!equal(op, TK_RESERVED, token)) {
        error_tok(token, "'%s'ではありません", op);
    }
    token = token->next;
}

static Token *expect_ident() {
    if (token->kind != TK_IDENT) {
        error_tok(token, "識別子ではありません");
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

static Macro *find_macro(Token *tok) {
    return hashmap_get(macros, tok->loc, tok->len);
}

static void add_macro(Token *tok, Macro *m) {
    hashmap_put(macros, tok->loc, tok->len, m);
}

char *search_include_paths(char *filename) {
    for (int i = 0; i < vector_size(include_paths); i++) {
        char *path = format("%s/%s", vector_get(include_paths, i), filename);
        if (access(path, R_OK) == 0) {
            return path;
        }
    }
    return NULL;
}

static char *read_include_filename(char **filename) {
    Token *tok;
    if ((tok = consume("<", TK_RESERVED))) {
        int len = 1;
        for (; !equal(">", TK_RESERVED, token); token = token->next) {
            len += token->len;
            if (token->has_space) {
                len++;
            }

            if (token->at_bol || at_eof(token)) {
                error_tok(tok, "'>'で閉じられていません");
            }
        }

        tok = tok->next;

        *filename = calloc(1, len);
        for (char *p = *filename; !equal(">", TK_RESERVED, tok);
             tok = tok->next) {
            if (tok->has_space) {
                *p++ = ' ';
            }
            memcpy(p, tok->loc, tok->len);
            p += tok->len;
        }

        return search_include_paths(*filename);
    }

    char *path;
    if (token->kind != TK_STR) {
        error_tok(token, "\"ファイル名\" ではありません");
    }

    *filename = token->str;
    if ((*filename)[0] == '/') {
        path = *filename;
        if (access(path, R_OK) != 0) {
            path = NULL;
        }
    } else {
        path = format("%s/%s", dirname(strdup(token->file->name)), *filename);
        if (access(path, R_OK) != 0) {
            path = search_include_paths(*filename);
        }
    }

    return path;
}

// tok2をtok1の末尾に連結する
Token *append(Token *tok1, Token *tok2) {
    if (!tok1 || tok1->kind == TK_EOF) {
        return tok2;
    }

    Token *cur = tok1;
    while (cur->next->kind != TK_EOF) {
        cur = cur->next;
    }
    cur->next = tok2;

    return tok1;
}

// 行末までのトークン列を複製する
static Token *copy_line(Token *tok) {
    Token head = {};
    Token *cur = &head;

    for (; !tok->at_bol; tok = tok->next) {
        cur = cur->next = copy_token(tok);
    }

    cur->next = eof_token;
    token = tok;
    return head.next;
}

static int64_t calc_const_expr_token(Token *tok) {
    tok = copy_line(tok);

    Token head = {};
    Token *cur = &head;
    for (Token *t = tok; !at_eof(t); t = t->next) {
        if (equal("defined", TK_IDENT, t)) {
            Token *start = t;
            t = t->next;
            bool has_paren = equal("(", TK_RESERVED, t);
            if (has_paren) {
                t = t->next;
            }
            if (t->kind != TK_IDENT) {
                error_tok(t, "識別子ではありません");
            }
            Macro *m = find_macro(t);
            if (has_paren) {
                t = t->next;
                if (!equal(")", TK_RESERVED, t)) {
                    error_tok(t, "')'ではありません");
                }
            }

            Token *replace = tokenize(new_file(start->file->name, m ? "1" : "0",
                                               start->file->number));
            cur = cur->next = replace;
            continue;
        }

        cur = cur->next = t;
    }

    tok = head.next;
    set_token_to_parse(tok);
    Node *node = conditional();
    return calc_const_expr(node, &(char *){NULL} /* dummy */);
}

static CondIncl *push_cond_incl(Token *tok, bool included) {
    CondIncl *ci = calloc(1, sizeof(CondIncl));
    ci->parent = cond_incl;
    ci->ctx = IN_THEN;
    ci->tok = tok;
    ci->included = included;
    cond_incl = ci;
    return ci;
}

static Token *skip_cond_incl() {
    while (!at_eof(token)) {
        if (equal("#", TK_RESERVED, token) &&
            (equal("elif", TK_IDENT, token->next) ||
             equal("else", TK_IDENT, token->next) ||
             equal("endif", TK_IDENT, token->next))) {
            return token;
        }

        if (equal("#", TK_RESERVED, token) &&
            (equal("if", TK_IDENT, token->next) ||
             equal("ifdef", TK_IDENT, token->next) ||
             equal("ifndef", TK_IDENT, token->next))) {
            token = token->next->next;
            token = skip_cond_incl();
            if (!at_eof(token)) {
                token = token->next;
            }
            continue;
        }

        token = token->next;
    }
    return token;
}

static MacroParam *read_params() {
    MacroParam head = {};
    MacroParam *cur = &head;

    while (!consume(")", TK_RESERVED)) {
        if (cur != &head) {
            expect(",");
        }

        MacroParam *param = calloc(1, sizeof(MacroParam));
        param->name = expect_ident();
        cur = cur->next = param;
    }

    return head.next;
}

static Token *read_arg() {
    Token head = {};
    Token *cur = &head;
    int depth = 0;

    while (!at_eof(token)) {
        if (equal("(", TK_RESERVED, token)) {
            depth++;
        }
        if (equal(")", TK_RESERVED, token)) {
            if (depth == 0) {
                break;
            }
            depth--;
        }
        if (equal(",", TK_RESERVED, token) && depth == 0) {
            break;
        }
        cur = cur->next = copy_token(token);
        token = token->next;
    }

    cur->next = eof_token;

    return head.next;
}

static Token *find_arg(List args, MacroParam *params, Token *tok) {
    ListIter it = list_begin(args);

    for (MacroParam *param = params; param;
         param = param->next, it = list_next(it)) {
        if (param->name->len == tok->len &&
            !memcmp(param->name->loc, tok->loc, tok->len)) {
            return *it;
        }
    }

    return NULL;
}

static Token *stringize(Token *hash, Token *arg) {
    int len = 1;
    for (Token *t = arg; !at_eof(t); t = t->next) {
        if (t != arg && t->has_space) {
            len++;
        }
        len += t->len;
    }

    char *buf = calloc(1, len);

    for (Token *t = arg; !at_eof(t); t = t->next) {
        if (t != arg && t->has_space) {
            buf = strcat(buf, " ");
        }
        strncat(buf, t->loc, t->len);
    }

    len = 3;
    for (char *p = buf; *p; p++) {
        if (*p == '\\' || *p == '"') {
            len++;
        }
        len++;
    }

    char *buf2 = calloc(1, len);

    char *p = buf, *q = buf2;
    *q++ = '"';
    while (*p) {
        if (*p == '\\' || *p == '"') {
            *q++ = '\\';
        }
        *q++ = *p++;
    }
    *q = '"';

    return tokenize(new_file(hash->file->name, buf2, hash->file->number));
}

static List collect_args(MacroParam *params) {
    List args = list_new();
    if (params) {
        for (MacroParam *param = params; param; param = param->next) {
            if (param != params) {
                expect(",");
            }

            Token *arg = read_arg();
            list_push_back(args, arg);
        }
    }

    return args;
}

static Token *paste(Token *lhs, Token *rhs) {
    char *buf = format("%.*s%.*s", lhs->len, lhs->loc, rhs->len, rhs->loc);
    return tokenize(new_file(lhs->file->name, buf, lhs->file->number));
}

// `tok`を編集して実際に置換される文字列に対応するTokenリストにする
static Token *subst(Token *tok, List args, MacroParam *params) {
    Token head = {};
    Token *cur = &head;
    for (Token *tok2 = tok; !at_eof(tok2); tok2 = tok2->next) {
        if (equal("#", TK_RESERVED, tok2)) {
            Token *hash = tok2;
            Token *arg = find_arg(args, params, tok2->next);
            if (!arg) {
                error_tok(tok2, "マクロのパラメータが後に続く必要があります");
            }

            cur = cur->next = stringize(hash, arg);
            tok2 = tok2->next;
            continue;
        }

        if (equal("##", TK_RESERVED, tok2)) {
            if (tok2 == tok) {
                error_tok(tok2, "置換規則の先頭で使うことはできません");
            }
            if (at_eof(tok2->next)) {
                error_tok(tok2, "置換規則の末尾で使うことはできません");
            }

            Token *arg = find_arg(args, params, tok2->next);
            if (arg) {
                if (!at_eof(arg)) {
                    *cur = *paste(cur, arg);
                    for (Token *t = arg->next; !at_eof(t); t = t->next) {
                        cur = cur->next = copy_token(t);
                    }
                }

                tok2 = tok2->next;
                continue;
            }

            *cur = *paste(cur, tok2->next);
            tok2 = tok2->next;
            continue;
        }

        Token *arg = find_arg(args, params, tok2);
        if (arg) {
            if (at_eof(arg) && equal("##", TK_RESERVED, tok2->next)) {
                Token *rhs = tok2->next->next;
                Token *arg2 = find_arg(args, params, rhs);
                if (arg2) {
                    for (Token *t = arg2; !at_eof(t); t = t->next) {
                        cur = cur->next = copy_token(t);
                    }
                } else {
                    cur = cur->next = copy_token(rhs);
                }
                tok2 = rhs;
                continue;
            }

            for (Token *t = arg; !at_eof(t); t = t->next) {
                cur = cur->next = copy_token(t);
            }
            continue;
        }

        cur = cur->next = tok2;
    }
    return head.next;
}

static bool expand_macro(Token **tok) {
    Macro *m = find_macro(token);
    if (!m) {
        return false;
    }

    // 一旦`#define M xxx`のxxxのトークンを複製しTokenリスト`replace`に保存する
    Token *replace;
    {
        Token head = {};
        Token *cur = &head;
        for (Token *t = m->body; !t->at_bol && !at_eof(t); t = t->next) {
            cur = cur->next = copy_token(t);
        }
        cur->next = eof_token;
        replace = head.next;
    }

    token = token->next;

    if (m->kind == FUNCLIKE && !consume("(", TK_RESERVED)) {
        return false;
    }

    List args = collect_args(m->params);
    replace = subst(replace, args, m->params);

    if (m->kind == FUNCLIKE) {
        expect(")");
    }

    for (Token *t = replace; !at_eof(t); t = t->next) {
        *tok = (*tok)->next = t;
    }

    return true;
}

void define_macro(char *name, char *buf) {
    Token *tok = tokenize(new_file("<built-in>", buf, 0));
    tok->at_bol = false;

    Macro *m = calloc(1, sizeof(Macro));
    m->kind = OBJLIKE;
    m->body = tok;

    hashmap_put(macros, name, (int)strlen(name), m);
}

void init_macros() {
    define_macro("__STDC__", "1");
    define_macro("__STDC_HOSTED__", "1");
    define_macro("__STDC_VERSION__", "201112L");
    define_macro("__STDC_NO_ATOMICS__", "1");
    define_macro("__STDC_NO_COMPLEX__", "1");
    define_macro("__STDC_NO_THREADS__", "1");
    define_macro("__STDC_NO_VLA__", "1");
    define_macro("__LP64__", "1");
    define_macro("_LP64", "1");
    define_macro("__SIZEOF_SHORT__", "2");
    define_macro("__SIZEOF_INT__", "4");
    define_macro("__SIZEOF_LONG__", "8");
    define_macro("__SIZEOF_LONG_LONG__", "8");
    define_macro("__SIZEOF_POINTER__", "8");
    define_macro("__SIZEOF_FLOAT__", "4");
    define_macro("__SIZEOF_DOUBLE__", "8");
    define_macro("__SIZEOF_LONG_DOUBLE__", "8");
    define_macro("__ELF__", "1");
    define_macro("__linux", "1");
    define_macro("__linux__", "1");
    define_macro("__gnu_linux__", "1");
    define_macro("linux", "1");
    define_macro("__unix", "1");
    define_macro("__unix__", "1");
    define_macro("unix", "1");
    define_macro("__amd64", "1");
    define_macro("__amd64__", "1");
    define_macro("__x86_64", "1");
    define_macro("__x86_64__", "1");
}

Token *preprocess2() {
    Token head = {};
    Token *cur = &head;

    while (!at_eof(token)) {
        Token *tok;

        if (expand_macro(&cur)) {
            continue;
        }

        if (!consume_hash()) {
            cur = cur->next = token;
            token = token->next;
            continue;
        }

        if (consume("include", TK_IDENT)) {
            Token *start = token;
            char *filename;
            char *path = read_include_filename(&filename);
            if (!path) {
                error_tok(start, "ファイルが見つかりません: %s", filename);
            }

            Token *tok2 = tokenize_file(path);
            token = append(tok2, token->next);
            continue;
        }

        if ((tok = consume("if", TK_IDENT))) {
            int64_t val = calc_const_expr_token(tok->next);
            push_cond_incl(tok, val);
            if (!val) {
                token = skip_cond_incl();
            }
            continue;
        }

        if ((tok = consume("elif", TK_IDENT))) {
            int64_t val = calc_const_expr_token(tok->next);
            if (!cond_incl || cond_incl->ctx == IN_ELSE) {
                error_tok(tok, "対応する#ifがありません");
            }

            cond_incl->ctx = IN_ELIF;
            if (cond_incl->included) {
                token = skip_cond_incl();
            } else if (val) {
                cond_incl->included = true;
            }
            continue;
        }

        if ((tok = consume("else", TK_IDENT))) {
            if (!cond_incl || cond_incl->ctx == IN_ELSE) {
                error_tok(tok, "対応する#ifがありません");
            }

            cond_incl->ctx = IN_ELSE;
            if (cond_incl->included) {
                token = skip_cond_incl();
            }
            continue;
        }

        if ((tok = consume("endif", TK_IDENT))) {
            if (!cond_incl) {
                error_tok(tok, "対応する#ifがありません");
            }

            cond_incl = cond_incl->parent;
            continue;
        }

        if ((tok = consume("ifdef", TK_IDENT))) {
            Macro *m = find_macro(token);
            token = token->next;
            push_cond_incl(tok, m);
            if (!m) {
                token = skip_cond_incl();
            }
            continue;
        }

        if ((tok = consume("ifndef", TK_IDENT))) {
            Macro *m = find_macro(token);
            token = token->next;
            push_cond_incl(tok, !m);
            if (m) {
                token = skip_cond_incl();
            }
            continue;
        }

        if (consume("define", TK_IDENT)) {
            bool is_objlike = true;
            Token *name = token;
            MacroParam *params = NULL;

            token = token->next;
            if (consume("(", TK_RESERVED)) {
                params = read_params();
                is_objlike = false;
            }

            Token *body = token;
            while (!token->at_bol) {
                token = token->next;
            }

            Macro *m = calloc(1, sizeof(Macro));
            m->kind = is_objlike ? OBJLIKE : FUNCLIKE;
            m->body = body;
            m->params = params;
            add_macro(name, m);
            continue;
        }

        if ((tok = consume("undef", TK_IDENT))) {
            hashmap_delete(macros, tok->next->loc, tok->next->len);
            token = token->next;
            continue;
        }

        if ((tok = consume("error", TK_IDENT))) {
            error_tok(tok, "#error");
        }

        if (token->at_bol) {
            continue;
        }

        error_tok(token, "不正なディレクティブです");
    }

    cur->next = token;
    return head.next;
}

Token *preprocess(Token *tok) {
    token = tok;
    init_macros();

    tok = preprocess2();

    if (cond_incl) {
        error_tok(cond_incl->tok, "対応する#endifがありません");
    }

    convert_keywords(tok);
    return tok;
}
