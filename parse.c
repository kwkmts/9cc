#include "9cc.h"

#ifndef ___DEBUG
//下の１行をアンコメントしてデバッグフラグを有効化
// #define ___DEBUG
#endif

//エラーを報告する関数
// printf()と同じ引数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//エラー箇所を報告
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

//
//トークナイザー
//

static bool at_eof() { return token->kind == TK_EOF; }

//新しいトークンを作成してcurに繋げる
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
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static bool is_ident2(char c) { return is_ident1(c) || ('0' <= c && c <= '9'); }

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') || (c == '_');
}

//入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        //空白文字はスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // if
        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        // return
        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
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
        if (strchr("+-*/()<>=;", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        //数値リテラル
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        //識別子
        if (is_ident1(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_ident2(*p));

            cur = new_token(TK_IDENT, cur, start++, p - start);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    cur = cur->next = new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//
//パーサー
//

//次のトークンが期待している記号の時は、トークンを1つ読み進めて真を返す
//それ以外の場合は偽を返す
static bool consume(char *op, TokenKind kind) {
    if (token->kind != kind || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

static Token *consume_ident() {
    if (token->kind != TK_IDENT) return NULL;
    Token *tok = token;
    token = token->next;
    return tok;
}

//次のトークンが期待している記号の時は、トークンを1つ読み進める
//それ以外の場合にはエラーを報告
static void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len))
        error_at(token->str, "'%s'ではありません", op);
    token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
//それ以外の場合にはエラーを報告
static int expect_number() {
    if (token->kind != TK_NUM) error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

//変数を名前で検索。見つからなければNULLを返す
static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next)
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    return NULL;
}

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node *new_node_var(Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;

    LVar *lvar = find_lvar(tok);
    if (lvar) {
        node->offset = lvar->offset;
    } else {
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = tok->str;
        lvar->len = tok->len;
        if (locals)
            lvar->offset = locals->offset + 8;
        else
            lvar->offset = 8;
        node->offset = lvar->offset;
        locals = lvar;
    }
    return node;
}

void program();
static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *primary();
static Node *unary();

// program = stmt*
void program() {
    int i = 0;
    while (!at_eof()) code[i++] = stmt();
    code[i] = NULL;
}

// stmt = expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "return" expr ";"
static Node *stmt() {
    Node *node;

#ifdef ___DEBUG
    printf("# debug:: (1)token->str: %s\n", token->str);
#endif

    if (consume("if", TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;

        if (consume("(", TK_RESERVED)) {
            node->cond = expr();
            expect(")");
        }

        node->then = stmt();

        if (consume("else", TK_ELSE)) {
            node->els = stmt();
        }

    } else if (consume("return", TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(";");
    } else {
        node = expr();
        expect(";");
    }

#ifdef ___DEBUG
    printf("# debug:: (2)token->str: %s\n", token->str);
#endif

    return node;
}

// expr = assign
static Node *expr() { return assign(); }

// assign = equality ("=" assign)?
static Node *assign() {
    Node *node = equality();

    if (consume("=", TK_RESERVED)) node = new_node(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==", TK_RESERVED))
            node = new_node(ND_EQ, node, relational());
        else if (consume("!=", TK_RESERVED))
            node = new_node(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<", TK_RESERVED))
            node = new_node(ND_LT, node, add());
        else if (consume("<=", TK_RESERVED))
            node = new_node(ND_LE, node, add());
        else if (consume(">", TK_RESERVED))
            node = new_node(ND_LT, add(), node);
        else if (consume(">=", TK_RESERVED))
            node = new_node(ND_LE, add(), node);
        else
            return node;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+", TK_RESERVED))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-", TK_RESERVED))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*", TK_RESERVED))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/", TK_RESERVED))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? unary | primary
static Node *unary() {
    if (consume("+", TK_RESERVED)) return unary();
    if (consume("-", TK_RESERVED))
        return new_node(ND_SUB, new_node_num(0), unary());
    return primary();
}

// primary = "(" expr ")" | ident | num
static Node *primary() {
    //次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(", TK_RESERVED)) {
        Node *node = expr();
        expect(")");
        return node;
    }

    //もしくは識別子のはず
    Token *tok = consume_ident();
    if (tok) return new_node_var(tok);

    //そうでなければ数値のはず
    return new_node_num(expect_number());
}