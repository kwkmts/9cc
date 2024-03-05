#include "9cc.h"

//
// ハッシュマップ(オープンアドレス法)の実装
//

#define HASHMAP_INIT_SIZE 16
#define HASHMAP_MAX_LOAD 75
#define TOMBSTONE ((void *)-1)

typedef struct {
    char *key;
    int keylen;
    void *val;
} HashmapEntry;

struct Hashmap {
    HashmapEntry *buckets;
    int capacity;
    int used;
};

Hashmap hashmap_new() {
    Hashmap map = calloc(1, sizeof(struct Hashmap));
    map->capacity = HASHMAP_INIT_SIZE;
    map->buckets = calloc(HASHMAP_INIT_SIZE, sizeof(HashmapEntry));
    return map;
}

static uint64_t fnv_hash(char *key, int keylen) {
    uint64_t hash = 0xcbf29ce484222325;
    for (int i = 0; i < keylen; i++) {
        hash *= 0x100000001b3;
        hash ^= *key++;
    }
    return hash;
}

static void rehash(Hashmap map) {
    int oldcap = map->capacity;
    HashmapEntry *oldbuckets = map->buckets;

    map->capacity *= 2;
    map->buckets = calloc(map->capacity, sizeof(HashmapEntry));
    map->used = 0;

    for (int i = 0; i < oldcap; i++) {
        HashmapEntry *e = &oldbuckets[i];
        if (e->key && e->key != TOMBSTONE) {
            hashmap_put(map, e->key, e->keylen, e->val);
        }
    }
}

static bool match(HashmapEntry *entry, char *key, int keylen) {
    return entry->key != TOMBSTONE && entry->keylen == keylen &&
           !memcmp(entry->key, key, keylen);
}

static HashmapEntry *get_entry(Hashmap map, char *key, int keylen) {
    uint64_t hash = fnv_hash(key, keylen);

    for (uint64_t i = hash % map->capacity; i < map->capacity;
         i = (i + 1) % map->capacity) {
        HashmapEntry *e = &map->buckets[i];
        if (!e->key) {
            return NULL;
        }
        if (match(e, key, keylen)) {
            return e;
        }
    }

    unreachable();
}

static HashmapEntry *get_or_insert_entry(Hashmap map, char *key, int keylen) {
    if (map->used * 100 / map->capacity >= HASHMAP_MAX_LOAD) {
        rehash(map);
    }

    uint64_t hash = fnv_hash(key, keylen);

    for (uint64_t i = hash % map->capacity;; i = (i + 1) % map->capacity) {
        HashmapEntry *e = &map->buckets[i];
        if (!e->key) {
            map->used++;
            e->key = key;
            e->keylen = keylen;
            return e;
        }
        if (e->key == TOMBSTONE) {
            e->key = key;
            e->keylen = keylen;
            return e;
        }
        if (match(e, key, keylen)) {
            return e;
        }
    }
}

void *hashmap_get(Hashmap map, char *key, int keylen) {
    HashmapEntry *e = get_entry(map, key, keylen);
    return e ? e->val : NULL;
}

void hashmap_put(Hashmap map, char *key, int keylen, void *val) {
    HashmapEntry *e = get_or_insert_entry(map, key, keylen);
    e->val = val;
}

void hashmap_delete(Hashmap map, char *key, int keylen) {
    HashmapEntry *e = get_entry(map, key, keylen);
    if (e) {
        e->key = TOMBSTONE;
    }
}

int hashmap_test() {
    Hashmap map = hashmap_new();

    for (int i = 0; i < 5000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }
    for (int i = 1000; i < 2000; i++) {
        char *key = format("key %d", i);
        hashmap_delete(map, key, (int)strlen(key));
    }
    for (int i = 1500; i < 1600; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }
    for (int i = 6000; i < 7000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }

    for (int i = 0; i < 1000; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 1000; i < 1500; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 1500; i < 1600; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 1600; i < 2000; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 2000; i < 5000; i++) {
        char *key = format("key %d", i);
        assert((size_t)hashmap_get(map, key, (int)strlen(key)) == i);
    }
    for (int i = 5000; i < 6000; i++) {
        assert(!hashmap_get(map, "no such key", 11));
    }
    for (int i = 6000; i < 7000; i++) {
        char *key = format("key %d", i);
        hashmap_put(map, key, (int)strlen(key), (void *)(size_t)i);
    }

    assert(!hashmap_get(map, "no such key", 11));

    exit(0);
}

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

static char *search_include_paths(char *filename) {
    for (int i = 0; i < vector_size(include_paths); i++) {
        char *path = format("%s/%s", vector_get(include_paths, i), filename);
        if (access(path, R_OK) == 0) {
            return path;
        }
    }
    return NULL;
}

// tok2をtok1の末尾に連結する
static Token *append(Token *tok1, Token *tok2) {
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
            Token *m = hashmap_get(macros, t->loc, t->len);
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

static Token *paste(Token *lhs, Token *rhs) {
    char *buf = format("%.*s%.*s", lhs->len, lhs->loc, rhs->len, rhs->loc);
    return tokenize(new_file(lhs->file->name, buf, lhs->file->number));
}

static bool expand_macro(Token **tok) {
    Macro *m = hashmap_get(macros, token->loc, token->len);
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

    List args = list_new();
    if (m->params) {
        for (MacroParam *param = m->params; param; param = param->next) {
            if (param != m->params) {
                expect(",");
            }

            Token *arg = read_arg();
            list_push_back(args, arg);
        }
    }

    // `replace`を編集して実際に置換される文字列に対応するTokenリストにする
    {
        Token head = {};
        Token *cur = &head;
        for (Token *tok2 = replace; !at_eof(tok2); tok2 = tok2->next) {
            if (equal("#", TK_RESERVED, tok2)) {
                Token *hash = tok2;
                Token *arg = find_arg(args, m->params, tok2->next);
                if (!arg) {
                    error_tok(tok2,
                              "マクロのパラメータが後に続く必要があります");
                }

                Token *t = stringize(hash, arg);
                cur = cur->next = t;
                tok2 = tok2->next;
                continue;
            }

            if (equal("##", TK_RESERVED, tok2)) {
                if (tok2 == replace) {
                    error_tok(tok2, "置換規則の先頭で使うことはできません");
                }
                if (at_eof(tok2->next)) {
                    error_tok(tok2, "置換規則の末尾で使うことはできません");
                }

                Token *arg = find_arg(args, m->params, tok2->next);
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

            Token *arg = find_arg(args, m->params, tok2);
            if (arg) {
                if (at_eof(arg) && equal("##", TK_RESERVED, tok2->next)) {
                    Token *rhs = tok2->next->next;
                    Token *arg2 = find_arg(args, m->params, rhs);
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
        replace = head.next;
    }

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

Token *preprocess(Token *tok) {
    token = tok;
    init_macros();

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
            char *path;
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

                Token *start = tok;
                tok = tok->next;

                char *buf = calloc(1, len);
                for (char *p = buf; !equal(">", TK_RESERVED, tok);
                     tok = tok->next) {
                    if (tok->has_space) {
                        *p++ = ' ';
                    }
                    memcpy(p, tok->loc, tok->len);
                    p += tok->len;
                }

                path = search_include_paths(buf);
                if (!path) {
                    error_tok(start, "ファイルが見つかりません: %s", buf);
                }
            } else {
                if (token->kind != TK_STR) {
                    error_tok(token, "\"ファイル名\" ではありません");
                }

                if (token->str[0] == '/') {
                    path = token->str;
                    if (access(path, R_OK) != 0) {
                        path = NULL;
                    }
                } else {
                    path = format("%s/%s", dirname(strdup(token->file->name)),
                                  token->str);
                    if (access(path, R_OK) != 0) {
                        path = search_include_paths(token->str);
                    }
                }

                if (!path) {
                    error_tok(token, "ファイルが見つかりません: %s",
                              token->str);
                }
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
            Token *m = hashmap_get(macros, token->loc, token->len);
            token = token->next;
            push_cond_incl(tok, m);
            if (!m) {
                token = skip_cond_incl();
            }
            continue;
        }

        if ((tok = consume("ifndef", TK_IDENT))) {
            Token *m = hashmap_get(macros, token->loc, token->len);
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
                MacroParam head2 = {};
                MacroParam *cur2 = &head2;

                while (!consume(")", TK_RESERVED)) {
                    if (cur2 != &head2) {
                        expect(",");
                    }

                    MacroParam *param = calloc(1, sizeof(MacroParam));
                    param->name = expect_ident();
                    cur2 = cur2->next = param;
                }

                is_objlike = false;
                params = head2.next;
            }

            Token *body = token;
            while (!token->at_bol) {
                token = token->next;
            }

            Macro *m = calloc(1, sizeof(Macro));
            m->kind = is_objlike ? OBJLIKE : FUNCLIKE;
            m->body = body;
            m->params = params;
            hashmap_put(macros, name->loc, name->len, m);
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

    if (cond_incl) {
        error_tok(cond_incl->tok, "対応する#endifがありません");
    }

    cur->next = token;

    convert_keywords(head.next);
    return head.next;
}
