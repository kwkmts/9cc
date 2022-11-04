#include "9cc.h"

#ifndef ___DEBUG
//下の１行をアンコメントしてデバッグフラグを有効化
// #define ___DEBUG
#endif

//
//パーサー
//

//次のトークンが期待している記号の時は、トークンを1つ読み進めて真を返す
//それ以外の場合は偽を返す
static bool consume(char *op, TokenKind kind) {
    if (token->kind != kind || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

//次のトークンが識別子の場合、トークンを1つ読み進めてその識別子を返す
//それ以外の場合はNULLを返す
static Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

//次のトークンが期待している記号の時は、トークンを1つ読み進める
//それ以外の場合にはエラーを報告
static void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

//次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
//それ以外の場合にはエラーを報告
static int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

//次のトークンが識別子の場合、トークンを1つ読み進めてその識別子を返す
//それ以外の場合にはエラーを報告
static Token *expect_ident() {
    Token *ident = consume_ident();
    if (!ident) {
        error_at(token->str, "識別子ではありません");
    }
    return ident;
}

static bool at_eof() { return token->kind == TK_EOF; }

//変数を名前で検索。見つからなければNULLを返す
static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
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

static Node *new_node_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *new_node_var(Token *tok) {
    Node *node = new_node(ND_LVAR);

    LVar *lvar = find_lvar(tok);
    if (lvar) {
        node->offset = lvar->offset;
    } else {
        lvar = calloc(1, sizeof(LVar));
        lvar->next = locals;
        lvar->name = tok->str;
        lvar->len = tok->len;
        node->offset = lvar->offset = locals ? locals->offset + 8 : 8;
        locals = lvar;
    }
    return node;
}

static Function *function();
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

// program = function-definition*
void program() {
    Function *cur = &prog;
    while (!at_eof()) {
        cur = cur->next = function();
    }
}

// function-definition = ident "(" (ident ("," ident)*)? ")" "{" compound-stmt
static Function *function() {
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));

    Token *tok = expect_ident();

    if (consume("(", TK_RESERVED)) {
        while (!consume(")", TK_RESERVED)) {
            if (consume(",", TK_RESERVED)) {
                continue;
            }

            Token *tok = expect_ident();

            new_node_var(tok);
        }
    };

    fn->name = strndup(tok->str, tok->len);
    fn->params = locals;

    if (consume("{", TK_RESERVED)) {
        fn->body = compound_stmt();
    }

    fn->locals = locals;

    return fn;
}

// stmt = expr? ";"
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
            node = new_node_binary(ND_ADD, node, mul());

        } else if (consume("-", TK_RESERVED)) {
            node = new_node_binary(ND_SUB, node, mul());

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

// unary = ("+" | "-")? unary | primary
static Node *unary() {
    if (consume("+", TK_RESERVED)) {
        return unary();
    }

    if (consume("-", TK_RESERVED)) {
        return new_node_binary(ND_SUB, new_node_num(0), unary());
    }

    return primary();
}

// primary = "(" expr ")" | ident ("(" (assign ("," assign)*)? ")")? | num
static Node *primary() {
    //次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(", TK_RESERVED)) {
        Node *node = expr();
        expect(")");
        return node;
    }

    //もしくは識別子のはず
    Token *tok = consume_ident();
    if (tok) {
        //関数呼び出し
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

        //変数
        return new_node_var(tok);
    }

    //そうでなければ数値のはず
    return new_node_num(expect_number());
}
