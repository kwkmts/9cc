#include "9cc.h"

#ifndef ___DEBUG
// 下の１行をアンコメントしてデバッグフラグを有効化
//  #define ___DEBUG
#endif

//
// パーサー
//

// 次のトークンが期待しているトークンの時は真を返し、それ以外では偽を返す
static bool equal(char *op, TokenKind kind) {
    return token->kind == kind && strlen(op) == token->len &&
           !memcmp(token->str, op, token->len);
}

// 次のトークンが期待しているトークンの時は、トークンを1つ読み進めて真を返す
// それ以外の場合は偽を返す
static bool consume(char *op, TokenKind kind) {
    if (!equal(op, kind)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてその識別子を返す
// それ以外の場合はNULLを返す
static Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

// 次のトークンが期待している記号の時は、トークンを1つ読み進める
// それ以外の場合にはエラーを報告
static void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
// それ以外の場合にはエラーを報告
static int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてその識別子を返す
// それ以外の場合にはエラーを報告
static Token *expect_ident() {
    Token *ident = consume_ident();
    if (!ident) {
        error_at(token->str, "識別子ではありません");
    }
    return ident;
}

static bool at_eof() { return token->kind == TK_EOF; }

// 変数を名前で検索。見つからなければNULLを返す
static Var *find_var(Token *tok) {
    for (Var *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }

    for (Var *var = globals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }

    return NULL;
}

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_unary(NodeKind kind, Node *expr) {
    Node *node = new_node(kind);
    node->lhs = expr;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *new_node_var(Var *var) {
    Node *node = new_node(ND_VAR);
    node->ty = var->ty;
    node->var = var;
    return node;
}

static Node *new_node_add(Node *lhs, Node *rhs) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_node_binary(ND_ADD, lhs, rhs);
    }

    if (lhs->ty->base && rhs->ty->base) {
        error("ポインタ同士の加算はできません");
    }

    // num + ptr => ptr + num
    if (is_integer(lhs->ty) && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->base->size));
    return new_node_binary(ND_ADD, lhs, rhs);
}

static Node *new_node_sub(Node *lhs, Node *rhs) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_node_binary(ND_SUB, lhs, rhs);
    }

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->base->size));
        return new_node_binary(ND_SUB, lhs, rhs);
    }

    // ptr - ptr (ポインタ間の要素数を返す)
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_node_binary(ND_SUB, lhs, rhs);
        node->ty = ty_int;
        return new_node_binary(ND_DIV, node, new_node_num(lhs->ty->base->size));
    }

    error("数値からポインタ値を引くことはできません");
}

static Var *new_var(Token *tok, char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->len = tok->len;
    var->ty = ty;
    return var;
}

static Var *new_lvar(Token *tok, char *name, Type *ty) {
    Var *var = new_var(tok, name, ty);
    var->next = locals;
    var->offset = locals ? locals->offset + ty->size : ty->size;
    var->is_lvar = true;
    locals = var;
    return var;
}

static Var *new_gvar(Token *tok, char *name, Type *ty) {
    Var *var = new_var(tok, name, ty);
    var->next = globals;
    var->is_lvar = false;
    globals = var;
    return var;
}

static Var *new_str_literal(Token *tok, char *str) {
    static int str_count = 0;
    char *bf = calloc(1, 16);
    sprintf(bf, ".Lstr%d", str_count++);
    Var *var = new_gvar(tok, bf, array_of(ty_char, tok->len + 1));
    var->init_data = str;
    return var;
}

static Type *declspec();
static Type *declarator(Type *ty);
static Function *function(Type *ty);
static Node *stmt();
static Node *compound_stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *primary();
static Node *unary();

// program = (declspec declarator ("(" function-definition | ";"))*
void program() {
    Function *cur = &prog;
    while (!at_eof()) {
        Type *basety = declspec();
        Type *ty = declarator(basety);

        // 関数定義
        if (consume("(", TK_RESERVED)) {
            cur = cur->next = function(ty);
            continue;
        }

        // グローバル変数
        new_gvar(ty->name, strndup(ty->name->str, ty->name->len), ty);
        expect(";");
    }
}

// declspec = "int" | "char"
static Type *declspec() {
    if (consume("int", TK_KEYWORD)) {
        return ty_int;
    }

    if (consume("char", TK_KEYWORD)) {
        return ty_char;
    }
}

// declarator = "*"* indent ("[" num "]")?
static Type *declarator(Type *ty) {
    while (consume("*", TK_RESERVED)) {
        ty = pointer_to(ty);
    }

    Token *tok = expect_ident();

    if (consume("[", TK_RESERVED)) {
        ty = array_of(ty, expect_number());
        expect("]");
    }

    ty->name = tok;
    return ty;
}

// function-definition = func-params? ")" "{" compound-stmt
// func-params = declspec declarator ("," declspec declarator)*
static Function *function(Type *ty) {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    fn->name = strndup(ty->name->str, ty->name->len);

    while (!consume(")", TK_RESERVED)) {
        if (consume(",", TK_RESERVED)) {
            continue;
        }

        Type *basety = declspec();
        Type *ty = declarator(basety);

        new_lvar(ty->name, strndup(ty->name->str, ty->name->len), ty);
    }

    fn->params = locals;

    if (consume("{", TK_RESERVED)) {
        fn->body = compound_stmt();
    }

    fn->locals = locals;

    return fn;
}

// stmt = expr? ";"
//      | declspec declarator ";"
//      | "{" compound-stmt
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "return" expr ";"
static Node *stmt() {
    Node *node;

#ifdef ___DEBUG
    printf("# debug:: (1)token->str: %s\n", token->str);
#endif

    if (consume("{", TK_RESERVED)) {
        node = compound_stmt();

    } else if (consume("if", TK_KEYWORD)) {
        node = new_node(ND_IF);

        if (consume("(", TK_RESERVED)) {
            node->cond = expr();
            expect(")");
        }

        node->then = stmt();

        if (consume("else", TK_KEYWORD)) {
            node->els = stmt();
        }

    } else if (consume("while", TK_KEYWORD)) {
        node = new_node(ND_LOOP);

        if (consume("(", TK_RESERVED)) {
            node->cond = expr();
            expect(")");
        }

        node->then = stmt();

    } else if (consume("for", TK_KEYWORD)) {
        node = new_node(ND_LOOP);

        if (consume("(", TK_RESERVED)) {
            if (!consume(";", TK_RESERVED)) {
                node->init = expr();
                expect(";");
            }

            if (!consume(";", TK_RESERVED)) {
                node->cond = expr();
                expect(";");
            }

            if (!consume(")", TK_RESERVED)) {
                node->after = expr();
                expect(")");
            }
        }

        node->then = stmt();

    } else if (consume("return", TK_KEYWORD)) {
        node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(";");

    } else if (consume(";", TK_RESERVED)) {
        node = new_node(ND_NULL_STMT);

    } else if (equal("int", TK_KEYWORD) || equal("char", TK_KEYWORD)) {
        Type *ty;
        if (consume("int", TK_KEYWORD)) {
            ty = declarator(ty_int);
        } else if (consume("char", TK_KEYWORD)) {
            ty = declarator(ty_char);
        }
        Var *var =
          new_lvar(ty->name, strndup(ty->name->str, ty->name->len), ty);
        node = new_node_var(var);
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

// compound-stmt = stmt* "}"
static Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK);
    Node head = {};

    for (Node *cur = &head; !consume("}", TK_RESERVED); cur = cur->next) {
        Node *node = calloc(1, sizeof(Node));
        node = stmt();
        cur->next = node;
        add_type(cur->next);
    }

    node->body = head.next;

    return node;
}

// expr = assign
static Node *expr() { return assign(); }

// assign = equality ("=" assign)?
static Node *assign() {
    Node *node = equality();

    if (consume("=", TK_RESERVED)) {
        node = new_node_binary(ND_ASSIGN, node, assign());
    }

    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume("==", TK_RESERVED)) {
            node = new_node_binary(ND_EQ, node, relational());

        } else if (consume("!=", TK_RESERVED)) {
            node = new_node_binary(ND_NE, node, relational());

        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume("<", TK_RESERVED)) {
            node = new_node_binary(ND_LT, node, add());

        } else if (consume("<=", TK_RESERVED)) {
            node = new_node_binary(ND_LE, node, add());

        } else if (consume(">", TK_RESERVED)) {
            node = new_node_binary(ND_LT, add(), node);

        } else if (consume(">=", TK_RESERVED)) {
            node = new_node_binary(ND_LE, add(), node);

        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+", TK_RESERVED)) {
            node = new_node_add(node, mul());

        } else if (consume("-", TK_RESERVED)) {
            node = new_node_sub(node, mul());

        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume("*", TK_RESERVED)) {
            node = new_node_binary(ND_MUL, node, unary());

        } else if (consume("/", TK_RESERVED)) {
            node = new_node_binary(ND_DIV, node, unary());

        } else {
            return node;
        }
    }
}

// unary = "sizeof" unary | ("+" | "-")? unary | ("*" | "&") unary | primary
static Node *unary() {
    if (consume("sizeof", TK_KEYWORD)) {
        Node *node = unary();
        add_type(node);
        return new_node_num(node->ty->size);
    }

    if (consume("+", TK_RESERVED)) {
        return unary();
    }

    if (consume("-", TK_RESERVED)) {
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    }

    if (consume("*", TK_RESERVED)) {
        return new_node_unary(ND_DEREF, unary());
    }

    if (consume("&", TK_RESERVED)) {
        return new_node_unary(ND_ADDR, unary());
    }

    return primary();
}

// ary_element = num "]"
static Node *ary_element(Node *var) {
    int idx = expect_number();
    expect("]");
    return new_node_unary(ND_DEREF, new_node_add(var, new_node_num(idx)));
}

// primary = "(" expr ")" | ident (("(" func-args? ")") | ("[" postfix))? | str ("[" postfix)? | num
// func-args = assign ("," assign)*
static Node *primary() {
    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(", TK_RESERVED)) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // 識別子
    Token *tok = consume_ident();
    if (tok) {
        // 関数呼び出し
        if (consume("(", TK_RESERVED)) {
            Token *start = tok;
            tok = tok->next->next;

            Node head = {};
            Node *cur = &head;

            while (!consume(")", TK_RESERVED)) {
                if (consume(",", TK_RESERVED)) {
                    continue;
                }
                cur = cur->next = assign();
            }

            Node *node = new_node(ND_FUNCALL);
            node->funcname = strndup(start->str, start->len);
            node->args = head.next;
            return node;
        }

        // 変数
        Var *var = find_var(tok);
        if (!var) {
            error("定義されていない変数です");
        }

        Node *node = new_node_var(var);

        if (consume("[", TK_RESERVED)) {
            return ary_element(node);
        }

        return node;
    }

    // 文字列リテラル
    if (token->kind == TK_STR) {
        Var *var = new_str_literal(token, strndup(token->str, token->len));
        Node *node = new_node_var(var);

        token = token->next;

        if (consume("[", TK_RESERVED)) {
            return ary_element(node);
        }

        return node;
    }

    // 数値
    return new_node_num(expect_number());
}
