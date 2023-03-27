#include "9cc.h"

//
// パーサー
//

static Node *gotos;          // 現在の関数内におけるgoto文のリスト
static Node *labels;         // 現在の関数内におけるラベルのリスト
static int cur_brk_label_id; // 現在のbreak文のジャンプ先ラベルID
static int cur_cont_label_id;// 現在のcontinue文のジャンプ先ラベルID
static Node *cur_switch;     // 現在パース中のswitch文ノード

// ブロックスコープの型
typedef struct Scope Scope;
struct Scope {
    Scope *parent;// 親のスコープ
    Var *vars;    // スコープに属する変数
    Type *tags;   // スコープに属するタグ
};

Scope *scope = &(Scope){};

// 初期化における指示子の型
typedef struct InitDesg InitDesg;
struct InitDesg {
    InitDesg *next;
    int idx;
    Member *member;
    Var *var;
};

// 次のトークンが期待しているトークンの時は真を返し、それ以外では偽を返す
static bool equal(char *op, TokenKind kind) {
    return token->kind == kind && strlen(op) == token->len &&
           !memcmp(token->str, op, token->len);
}

// 次のトークンが期待しているトークンの時は、トークンを1つ読み進めてそのトークンを返す
// それ以外の場合はNULLを返す
static Token *consume(char *op, TokenKind kind) {
    if (!equal(op, kind)) {
        return NULL;
    }
    Token *tok = token;
    token = token->next;
    return tok;
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

// 次のトークンが期待している記号の時は、トークンを1つ読み進めてその記号を返す
// それ以外の場合にはエラーを報告
static Token *expect(char *op) {
    if (!equal(op, TK_RESERVED)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
// それ以外の場合にはエラーを報告
static Token *expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    Token *tok = token;
    token = token->next;
    return tok;
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

static void enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->parent = scope;
    scope = sc;
}

static void leave_scope() {
    scope = scope->parent;
}

int64_t calc_const_expr(Node *node) {
    switch (node->kind) {
    case ND_ADD:
        return calc_const_expr(node->lhs) + calc_const_expr(node->rhs);
    case ND_SUB:
        return calc_const_expr(node->lhs) - calc_const_expr(node->rhs);
    case ND_MUL:
        return calc_const_expr(node->lhs) * calc_const_expr(node->rhs);
    case ND_DIV:
        return calc_const_expr(node->lhs) / calc_const_expr(node->rhs);
    case ND_EQ:
        return calc_const_expr(node->lhs) == calc_const_expr(node->rhs);
    case ND_NE:
        return calc_const_expr(node->lhs) != calc_const_expr(node->rhs);
    case ND_LT:
        return calc_const_expr(node->lhs) < calc_const_expr(node->rhs);
    case ND_LE:
        return calc_const_expr(node->lhs) <= calc_const_expr(node->rhs);
    case ND_NUM:
        return node->val;
    default:
        error_tok(node->tok, "初期化子が定数ではありません");
    }
}

// 変数を名前で検索。見つからなければNULLを返す
static Var *find_var(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->parent) {
        for (Var *var = sc->vars; var; var = var->scope_next) {
            if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
                return var;
            }
        }
    }
    return NULL;
}

static Function *find_func(Token *tok) {
    for (Function *fn = functions; fn; fn = fn->next) {
        if (fn->len == tok->len && !memcmp(tok->str, fn->name, fn->len)) {
            return fn;
        }
    }
    return NULL;
}

static Type *find_tag(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->parent) {
        for (Type *tag = sc->tags; tag; tag = tag->scope_next) {
            if (tag->name->len == tok->len && !memcmp(tok->str, tag->name->str, tag->name->len)) {
                return tag;
            }
        }
    }
    return NULL;
}

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

static Node *new_node_num(int64_t val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_node_cast(Node *expr, Type *ty) {
    add_type(expr);

    Node *node = new_node(ND_CAST, ty->ident);
    node->lhs = expr;
    node->ty = copy_type(ty);
    return node;
}

static Node *new_node_var(Var *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->ty = var->ty;
    node->var = var;
    return node;
}

static Node *new_node_member(Node *lhs, Member *member, Token *tok) {
    Node *node = new_node(ND_MEMBER, tok);
    node->lhs = lhs;
    node->member = member;
    return node;
}

static Node *new_node_add(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_node_binary(ND_ADD, lhs, rhs, tok);
    }

    if (lhs->ty->base && rhs->ty->base) {
        error_at(tok->str, "ポインタ同士の加算はできません");
    }

    // num + ptr => ptr + num
    if (is_integer(lhs->ty) && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->base->size, NULL), NULL);
    return new_node_binary(ND_ADD, lhs, rhs, tok);
}

static Node *new_node_sub(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_node_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - num
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->base->size, NULL), NULL);
        return new_node_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - ptr (ポインタ間の要素数を返す)
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_node_binary(ND_SUB, lhs, rhs, tok);
        node->ty = ty_int;
        return new_node_binary(ND_DIV, node, new_node_num(lhs->ty->base->size, NULL), NULL);
    }

    error_at(tok->str, "数値からポインタ値を引くことはできません");
}

static Var *new_var(char *str, int len, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = strndup(str, len);
    var->len = len;
    var->ty = ty;

    // 変数とスコープを紐付ける
    var->scope_next = scope->vars;
    scope->vars = var;

    return var;
}

static Var *new_lvar(char *str, int len, Type *ty) {
    Var *var = new_var(str, len, ty);
    var->next = locals;
    var->offset = align_to(locals ? locals->offset + ty->size : ty->size, ty->align);
    var->is_lvar = true;
    locals = var;
    return var;
}

static Var *new_gvar(char *str, int len, Type *ty) {
    Var *var = new_var(str, len, ty);
    var->next = globals;
    var->is_lvar = false;
    globals = var;
    return var;
}

static Function *new_func(char *str, int len, Type *ty) {
    Function *fn = calloc(1, sizeof(Function));
    fn->next = functions;
    fn->name = strndup(str, len);
    fn->len = len;
    fn->ty = ty;
    fn->has_definition = false;
    functions = fn;
    return fn;
}

static Var *new_str_literal(Token *tok, char *str) {
    static int str_count = 0;
    char *bf = calloc(1, 16);
    sprintf(bf, ".Lstr%d", str_count++);
    Var *var = new_gvar(bf, strlen(bf), array_of(ty_char, tok->len + 1));
    var->init_data_str = str;
    return var;
}

// A op= B を `tmp = &A, *tmp = *tmp op B` に変換する
static Node *to_assign(Node *binary) {
    add_type(binary->lhs);
    add_type(binary->rhs);

    Node *var = new_node_var(new_lvar("", 0, pointer_to(binary->lhs->ty)), NULL);

    Node *expr1 = new_node_binary(ND_ASSIGN,
                                  var,
                                  new_node_unary(ND_ADDR, binary->lhs, NULL),
                                  NULL);

    Node *expr2 = new_node_binary(ND_ASSIGN,
                                  new_node_unary(ND_DEREF, var, NULL),
                                  new_node_binary(binary->kind,
                                                  new_node_unary(ND_DEREF, var, NULL),
                                                  binary->rhs,
                                                  NULL),
                                  NULL);

    return new_node_binary(ND_COMMA, expr1, expr2, NULL);
}

static Initializer *new_initializer(Type *ty, bool is_flexible) {
    Initializer *init = calloc(1, sizeof(Initializer));
    init->ty = ty;

    if (is_type_of(TY_ARY, ty)) {
        if (is_flexible && ty->ary_len < 0) {
            init->is_flexible = true;
            return init;
        }

        init->children = calloc(ty->ary_len, sizeof(Initializer *));
        for (int i = 0; i < ty->ary_len; i++) {
            init->children[i] = new_initializer(ty->base, false);
        }

        return init;
    }

    if (is_type_of(TY_STRUCT, ty)) {
        int len = 0;
        for (Member *mem = ty->members; mem; mem = mem->next) {
            len++;
        }

        init->children = calloc(len, sizeof(Initializer *));
        for (Member *mem = ty->members; mem; mem = mem->next) {
            init->children[mem->idx] = new_initializer(mem->ty, false);
        }

        return init;
    }

    return init;
}

static Type *declspec();
static Type *declarator(Type *ty);
static void function(Type *ty);
static Node *lvar_initializer(Var *var);
static void gvar_initializer(Var *var);
static Node *declaration();
static Node *stmt();
static Node *compound_stmt();
static Node *expr();
static Node *assign();
static Node *conditional();
static Node *logor();
static Node *logand();
static Node *bitor();
static Node *bitxor();
static Node *bitand();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *cast();
static Node *unary();
static Node *postfix();
static Node *primary();

// program = global-declaration*
// global-declaration = declspec declarator (function-decl | (("=" initializer)? ";"))
//                    | declspec ";"
void program() {
    while (!at_eof()) {
        Type *basety = declspec();

        if (consume(";", TK_RESERVED)) {
            continue;
        }

        Type *ty = declarator(basety);

        // 関数定義
        if (is_type_of(TY_FUNC, ty)) {
            function(ty);
            continue;
        }

        // グローバル変数
        Var *var = new_gvar(ty->ident->str, ty->ident->len, ty);
        if (consume("=", TK_RESERVED)) {
            gvar_initializer(var);
        }
        expect(";");
    }
}

// struct-decl = ident? "{" (declspec declarator ";")* "}"
//             | ident
static Type *struct_decl() {
    Token *tag = consume_ident();
    if (tag && !equal("{", TK_RESERVED)) {
        Type *ty = find_tag(tag);
        if (!ty) {
            error_tok(tag, "そのような構造体型はありません");
        }
        return ty;
    }

    expect("{");
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_STRUCT;
    ty->align = 1;

    Member head = {};
    Member *cur = &head;

    int offset = 0;
    int idx = 0;
    while (!consume("}", TK_RESERVED)) {
        Member *mem = calloc(1, sizeof(Member));
        Type *ty2 = declarator(declspec());
        expect(";");

        mem->idx = idx++;
        mem->ty = ty2;
        mem->name = ty2->ident;
        mem->offset = offset = align_to(offset, mem->ty->align);
        offset += ty2->size;

        if (ty->align < mem->ty->align) {
            ty->align = mem->ty->align;
        }

        cur = cur->next = mem;
    }

    ty->members = head.next;
    ty->size = align_to(offset, ty->align);

    // タグとスコープを紐付ける
    if (tag) {
        ty->name = tag;
        ty->scope_next = scope->tags;
        scope->tags = ty;
    }

    return ty;
}

// declspec = "void" | "int" | "char" | "short" | "long" | "struct"
static Type *declspec() {
    if (consume("void", TK_KEYWORD)) {
        return ty_void;
    }

    if (consume("int", TK_KEYWORD)) {
        return ty_int;
    }

    if (consume("char", TK_KEYWORD)) {
        return ty_char;
    }

    if (consume("short", TK_KEYWORD)) {
        return ty_short;
    }

    if (consume("long", TK_KEYWORD)) {
        return ty_long;
    }

    if (consume("struct", TK_KEYWORD)) {
        return struct_decl();
    }
}

// type-suffix = "[" num "]" type-suffix? | "(" func-params? ")" | ε
// func-params = declspec declarator ("," declspec declarator)*
static Type *type_suffix(Type *ty) {
    if (consume("[", TK_RESERVED)) {
        int size = token->kind == TK_NUM ? expect_number()->val : -1;
        expect("]");
        ty = type_suffix(ty);
        return array_of(ty, size);
    }

    if (consume("(", TK_RESERVED)) {
        Type head = {};
        Type *cur = &head;

        while (!consume(")", TK_RESERVED)) {
            if (consume(",", TK_RESERVED)) {
                continue;
            }

            cur = cur->next = copy_type(declarator(declspec()));
        }

        return func_type(ty, head.next);
    }

    return ty;
}

// declarator = "*"* (ident | "(" declarator ")") type-suffix
static Type *declarator(Type *ty) {
    while (consume("*", TK_RESERVED)) {
        ty = pointer_to(ty);
    }

    if (consume("(", TK_RESERVED)) {
        Token *start = token;

        Type dummy = {};
        declarator(&dummy);
        expect(")");
        ty = type_suffix(ty);

        token = start;
        ty = declarator(ty);
        expect(")");
        type_suffix(&dummy);
        return ty;
    }

    Token *tok = expect_ident();
    ty = type_suffix(ty);
    ty->ident = tok;
    return ty;
}

// abstract-declarator = "*"* ("(" abstract-declarator ")")? type-suffix
static Type *abstract_declarator(Type *ty) {
    while (consume("*", TK_RESERVED)) {
        ty = pointer_to(ty);
    }

    if (consume("(", TK_RESERVED)) {
        Token *start = token;

        Type dummy = {};
        abstract_declarator(&dummy);
        expect(")");
        ty = type_suffix(ty);

        token = start;
        ty = abstract_declarator(ty);
        expect(")");
        type_suffix(&dummy);
        return ty;
    }

    return type_suffix(ty);
}

static void resolve_goto_labels() {
    for (Node *x = gotos; x; x = x->goto_next) {
        for (Node *y = labels; y; y = y->label_next) {
            if (strcmp(x->label, y->label) == 0) {
                x->label_id = y->label_id;
                break;
            }
        }

        if (!x->label_id) {
            error_tok(x->tok->next, "そのようなラベルはありません");
        }
    }
}

// function-decl = ("{" compound-stmt) | ";"
static void function(Type *ty) {
    Function *fn = find_func(ty->ident);
    if (!fn) {
        fn = new_func(ty->ident->str, ty->ident->len, ty);
    }

    if (consume(";", TK_RESERVED)) {
        return;
    }

    locals = NULL;
    enter_scope();
    for (Type *param = ty->params; param; param = param->next) {
        new_lvar(param->ident->str, param->ident->len, param);
    }

    fn->params = locals;
    expect("{");
    fn->body = compound_stmt();
    fn->locals = locals;
    fn->has_definition = true;
    leave_scope();
    resolve_goto_labels();
}

static void initializer2(Initializer *init);

static int count_ary_init_elems(Type *ty) {
    Token *tmp = token;// 要素数を数える直前のトークンのアドレスを退避
    Initializer *dummy = new_initializer(ty->base, false);
    int i = 0;
    while (!equal("}", TK_RESERVED)) {
        if (consume(",", TK_RESERVED)) {
            continue;
        }
        initializer2(dummy);
        i++;
    }

    token = tmp;// 現在着目しているトークンを元に戻す
    return i;
}

static void initialize_with_zero(Initializer *init) {
    if (is_type_of(TY_ARY, init->ty)) {
        for (int i = 0; i < init->ty->ary_len; i++) {
            initialize_with_zero(init->children[i]);
        }
        return;
    }

    if (is_type_of(TY_STRUCT, init->ty)) {
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            initialize_with_zero(init->children[mem->idx]);
        }
        return;
    }

    init->expr = new_node_num(0, NULL);
}

// initializer = "{" (initializer ("," initializer)*)? "}"
//             | assign
static void initializer2(Initializer *init) {
    if (is_type_of(TY_ARY, init->ty)) {
        if (consume("{", TK_RESERVED)) {
            if (init->is_flexible) {
                *init = *new_initializer(array_of(init->ty->base, count_ary_init_elems(init->ty)),
                                         false);
            }

            int i = 0;
            while (i < init->ty->ary_len && !equal("}", TK_RESERVED)) {
                if (consume(",", TK_RESERVED)) {
                    continue;
                }

                initializer2(init->children[i++]);
            }

            expect("}");
            return;
        }

        return;
    }

    if (is_type_of(TY_STRUCT, init->ty)) {
        if (!consume("{", TK_RESERVED)) {
            Node *expr = assign();
            add_type(expr);
            if (is_type_of(TY_STRUCT, expr->ty)) {
                init->expr = expr;
                return;
            }
        }

        Member *mem = init->ty->members;
        while (mem && !equal("}", TK_RESERVED)) {
            if (consume(",", TK_RESERVED)) {
                continue;
            }

            initializer2(init->children[mem->idx]);
            mem = mem->next;
        }

        expect("}");
        return;
    }

    init->expr = assign();
}

static Initializer *initializer(Type *ty, Type **new_ty) {
    Initializer *init = new_initializer(ty, true);
    initialize_with_zero(init);
    initializer2(init);
    *new_ty = init->ty;
    return init;
}

static Node *init_designator(InitDesg *desg) {
    if (desg->var) {
        return new_node_var(desg->var, NULL);
    }

    if (desg->member) {
        Node *node = new_node_member(init_designator(desg->next), desg->member, NULL);
        return node;
    }

    return new_node_unary(ND_DEREF,
                          new_node_add(init_designator(desg->next),
                                       new_node_num(desg->idx, NULL),
                                       NULL),
                          NULL);
}

static Node *create_lvar_init(Initializer *init, InitDesg *desg) {
    if (is_type_of(TY_ARY, init->ty)) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (int i = 0; i < init->ty->ary_len; i++) {
            InitDesg desg2 = {desg, i};
            node = new_node_binary(ND_COMMA,
                                   node,
                                   create_lvar_init(init->children[i], &desg2),
                                   NULL);
        }

        return node;
    }

    if (is_type_of(TY_STRUCT, init->ty) && !init->expr) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            InitDesg desg2 = {desg, 0, mem};
            node = new_node_binary(ND_COMMA,
                                   node,
                                   create_lvar_init(init->children[mem->idx], &desg2),
                                   NULL);
        }

        return node;
    }

    return new_node_binary(ND_ASSIGN, init_designator(desg), init->expr, NULL);
}

static Node *lvar_initializer(Var *var) {
    Initializer *init = initializer(var->ty, &var->ty);
    // 配列の要素数が省略された場合、var->ty->sizeがこの時点で確定するのでvar->offsetを更新
    var->offset = locals->next ? locals->next->offset + var->ty->size : var->ty->size;
    InitDesg desg = {NULL, 0, NULL, var};
    return create_lvar_init(init, &desg);
}

static void gvar_initializer(Var *var) {
    var->init = initializer(var->ty, &var->ty);
}

// declaration = declspec declarator ("=" initializer)? ";"
//             | declspec ";"
static Node *declaration() {
    Node head = {};
    Node *cur = &head;

    Type *basety = declspec();

    if (!equal(";", TK_RESERVED)) {
        Type *ty = declarator(basety);
        if (is_type_of(TY_VOID, ty) && !is_type_of(TY_FUNC, ty)) {
            error_tok(ty->ident, "void型の変数を宣言することはできません");
        }
        Var *var = new_lvar(ty->ident->str, ty->ident->len, ty);

        // 初期化
        if (consume("=", TK_RESERVED)) {
            Node *node = new_node(ND_INIT, NULL);
            node->lhs = lvar_initializer(var);
            cur->next = node;
        }

        if (var->ty->ary_len < 0) {
            error_at(ty->ident->str, "配列の要素数が指定されていません");
        }
    }

    expect(";");

    Node *node = new_node(ND_BLOCK, NULL);
    node->body = head.next;
    return node;
}

static bool is_typename() {
    static char *kw[] = {"void", "int", "char", "short", "long", "struct"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(kw[i], TK_KEYWORD)) {
            return true;
        }
    }
    return false;
}

static Node *expr_stmt() {
    Node *lhs = expr();
    Node *node = new_node(ND_EXPR_STMT, expect(";"));
    node->lhs = lhs;
    return node;
}

// stmt = expr-stmt
//      | ";"
//      | "{" compound-stmt
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "switch" "(" expr ")" stmt
//      | "case" num ":" stmt
//      | "default" ":" stmt
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "goto" ident ";"
//      | ident ":" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "return" expr ";"
static Node *stmt() {
    Node *node;

#ifdef ___DEBUG
    printf("# debug:: (1)token->str: %s\n", token->str);
#endif

    if (consume("{", TK_RESERVED)) {
        node = compound_stmt();

    } else if (equal("if", TK_KEYWORD)) {
        node = new_node(ND_IF, consume("if", TK_KEYWORD));

        if (consume("(", TK_RESERVED)) {
            node->cond = expr();
            expect(")");
        }

        node->then = stmt();

        if (consume("else", TK_KEYWORD)) {
            node->els = stmt();
        }

    } else if (equal("switch", TK_KEYWORD)) {
        node = new_node(ND_SWITCH, consume("switch", TK_KEYWORD));
        expect("(");
        node->cond = expr();
        expect(")");

        Node *sw = cur_switch;
        cur_switch = node;

        int brk_label_id = cur_brk_label_id;
        node->brk_label_id = cur_brk_label_id = count();
        node->then = stmt();

        cur_switch = sw;
        cur_brk_label_id = brk_label_id;

    } else if (equal("case", TK_KEYWORD)) {
        Token *tok = consume("case", TK_KEYWORD);
        int64_t val = expect_number()->val;

        if (!cur_switch) {
            error_tok(tok, "ここでcase文を使用することはできません");
        }

        for (Node *n = cur_switch->case_next; n; n = n->case_next) {
            if (val == n->case_val) {
                error_tok(tok, "重複したcase文");
            }
        }

        expect(":");

        node = new_node(ND_CASE, tok);
        node->case_val = val;
        node->label_id = count();
        node->lhs = stmt();
        node->case_next = cur_switch->cases;
        cur_switch->cases = node;

    } else if (equal("default", TK_KEYWORD)) {
        Token *tok = consume("default", TK_KEYWORD);
        if (!cur_switch) {
            error_tok(tok, "ここでdefault文を使用することはできません");
        }
        if (cur_switch->default_case) {
            error_tok(tok, "重複したdefault文");
        }

        expect(":");

        node = new_node(ND_CASE, tok);
        node->label_id = count();
        node->lhs = stmt();
        cur_switch->default_case = node;

    } else if (equal("while", TK_KEYWORD)) {
        node = new_node(ND_LOOP, consume("while", TK_KEYWORD));

        if (consume("(", TK_RESERVED)) {
            node->cond = expr();
            expect(")");
        }

        int brk_label_id = cur_brk_label_id;
        int cont_label_id = cur_cont_label_id;
        node->brk_label_id = cur_brk_label_id = count();
        node->cont_label_id = cur_cont_label_id = count();
        node->then = stmt();
        cur_brk_label_id = brk_label_id;
        cur_cont_label_id = cont_label_id;

    } else if (equal("for", TK_KEYWORD)) {
        node = new_node(ND_LOOP, consume("for", TK_KEYWORD));
        expect("(");

        enter_scope();

        if (!consume(";", TK_RESERVED)) {
            node->init = is_typename() ? declaration() : expr_stmt();
        }

        if (!consume(";", TK_RESERVED)) {
            node->cond = expr();
            expect(";");
        }

        if (!consume(")", TK_RESERVED)) {
            node->after = expr();
            expect(")");
        }

        int brk_label_id = cur_brk_label_id;
        int cont_label_id = cur_cont_label_id;
        node->brk_label_id = cur_brk_label_id = count();
        node->cont_label_id = cur_cont_label_id = count();
        node->then = stmt();

        leave_scope();

        cur_brk_label_id = brk_label_id;
        cur_cont_label_id = cont_label_id;

    } else if (equal("goto", TK_KEYWORD)) {
        node = new_node(ND_GOTO, consume("goto", TK_KEYWORD));
        Token *tok = expect_ident();
        node->label = strndup(tok->str, tok->len);
        node->goto_next = gotos;
        gotos = node;
        expect(";");

    } else if (equal("break", TK_KEYWORD)) {
        Token *tok = consume("break", TK_KEYWORD);
        if (!cur_brk_label_id) {
            error_tok(tok, "ここでbreak文を使用することはできません");
        }
        node = new_node(ND_GOTO, tok);
        node->label_id = cur_brk_label_id;
        expect(";");

    } else if (equal("continue", TK_KEYWORD)) {
        Token *tok = consume("continue", TK_KEYWORD);
        if (!cur_cont_label_id) {
            error_tok(tok, "ここでcontinue文を使用することはできません");
        }
        node = new_node(ND_GOTO, tok);
        node->label_id = cur_cont_label_id;
        expect(";");

    } else if (equal("return", TK_KEYWORD)) {
        node = new_node(ND_RETURN, consume("return", TK_KEYWORD));
        node->lhs = expr();
        expect(";");

    } else if (equal(";", TK_RESERVED)) {
        node = new_node(ND_NULL_STMT, consume(";", TK_RESERVED));

    } else {
        Token *tmp = token;
        Token *tok = consume_ident();
        if (tok && consume(":", TK_RESERVED)) {
            node = new_node(ND_LABEL, tok);
            node->label = strndup(tok->str, tok->len);
            node->label_id = count();
            node->lhs = stmt();
            node->label_next = labels;
            labels = node;

        } else {
            token = tmp;
            node = expr_stmt();
        }
    }

#ifdef ___DEBUG
    printf("# debug:: (2)token->str: %s\n", token->str);
#endif

    return node;
}

// compound-stmt = (stmt | (declspec declarator ("=" (expr | "{" (expr ("," expr)*)? "}"))? ";"))* "}"
static Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, NULL);
    Node head = {};

    enter_scope();

    for (Node *cur = &head; !consume("}", TK_RESERVED);) {
        if (is_typename()) {
            cur = cur->next = declaration();
        } else {
            cur = cur->next = stmt();
        }
        add_type(cur);
    }

    leave_scope();

    node->body = head.next;

    return node;
}

// expr = assign ("," expr)?
static Node *expr() {
    Node *node = assign();
    Token *tok;

    if ((tok = consume(",", TK_RESERVED))) {
        node = new_node_binary(ND_COMMA, node, expr(), tok);
    }

    return node;
}

// assign = conditional (("=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=") assign)?
static Node *assign() {
    Node *node = conditional();
    Token *tok;

    if ((tok = consume("=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node, assign(), tok);
    }

    if ((tok = consume("+=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_add(node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("-=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_sub(node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("*=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_MUL, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("/=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_DIV, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("%=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_MOD, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("&=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_BITAND, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("|=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_BITOR, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("^=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN,
                               node,
                               new_node_binary(ND_BITXOR, node, assign(), NULL),
                               tok);
    }

    return node;
}

// conditional = logor ("?" expr ":" conditional)?
static Node *conditional() {
    Node *cond = logor();
    Token *tok;

    if (!(tok = consume("?", TK_RESERVED))) {
        return cond;
    }

    Node *node = new_node(ND_COND, tok);
    node->cond = cond;
    node->then = expr();
    expect(":");
    node->els = conditional();
    return node;
}

// logor = logand ("||" logand)*
static Node *logor() {
    Node *node = logand();
    Token *tok;

    for (;;) {
        if ((tok = consume("||", TK_RESERVED))) {
            node = new_node_binary(ND_LOGOR, node, logand(), tok);
        } else {
            return node;
        }
    }
}

// logand = bitor ("&&" bitor)*
static Node *logand() {
    Node *node = bitor();
    Token *tok;

    for (;;) {
        if ((tok = consume("&&", TK_RESERVED))) {
            node = new_node_binary(ND_LOGAND, node, bitor(), tok);
        } else {
            return node;
        }
    }
}

// bitor = bitxor ("|" bitxor)*
static Node *bitor() {
    Node *node = bitxor();
    Token *tok;

    for (;;) {
        if ((tok = consume("|", TK_RESERVED))) {
            node = new_node_binary(ND_BITOR, node, bitxor(), tok);
        } else {
            return node;
        }
    }
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor() {
    Node *node = bitand();
    Token *tok;

    for (;;) {
        if ((tok = consume("^", TK_RESERVED))) {
            node = new_node_binary(ND_BITXOR, node, bitand(), tok);
        } else {
            return node;
        }
    }
}

// bitand = equality ("&" equality)*
static Node *bitand() {
    Node *node = equality();
    Token *tok;

    for (;;) {
        if ((tok = consume("&", TK_RESERVED))) {
            node = new_node_binary(ND_BITAND, node, equality(), tok);
        } else {
            return node;
        }
    }
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
    Node *node = relational();
    Token *tok;

    for (;;) {
        if ((tok = consume("==", TK_RESERVED))) {
            node = new_node_binary(ND_EQ, node, relational(), tok);

        } else if ((tok = consume("!=", TK_RESERVED))) {
            node = new_node_binary(ND_NE, node, relational(), tok);

        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
    Node *node = add();
    Token *tok;

    for (;;) {
        if ((tok = consume("<", TK_RESERVED))) {
            node = new_node_binary(ND_LT, node, add(), tok);

        } else if ((tok = consume("<=", TK_RESERVED))) {
            node = new_node_binary(ND_LE, node, add(), tok);

        } else if ((tok = consume(">", TK_RESERVED))) {
            node = new_node_binary(ND_LT, add(), node, tok);

        } else if ((tok = consume(">=", TK_RESERVED))) {
            node = new_node_binary(ND_LE, add(), node, tok);

        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
    Node *node = mul();
    Token *tok;

    for (;;) {
        if ((tok = consume("+", TK_RESERVED))) {
            node = new_node_add(node, mul(), tok);

        } else if ((tok = consume("-", TK_RESERVED))) {
            node = new_node_sub(node, mul(), tok);

        } else {
            return node;
        }
    }
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul() {
    Node *node = cast();
    Token *tok;

    for (;;) {
        if ((tok = consume("*", TK_RESERVED))) {
            node = new_node_binary(ND_MUL, node, cast(), tok);

        } else if ((tok = consume("/", TK_RESERVED))) {
            node = new_node_binary(ND_DIV, node, cast(), tok);

        } else if ((tok = consume("%", TK_RESERVED))) {
            node = new_node_binary(ND_MOD, node, cast(), tok);
        } else {
            return node;
        }
    }
}

// cast = "(" declspec abstract-declarator ")" cast
//      | unary
static Node *cast() {
    Token *tmp = token;
    if (consume("(", TK_RESERVED)) {
        if (is_typename()) {
            Type *ty = abstract_declarator(declspec());
            expect(")");
            return new_node_cast(cast(), ty);
        }
    }

    token = tmp;
    return unary();
}

// unary = "sizeof" unary
//       | "sizeof" "(" (declspec abstract-declarator) | expr ")"
//       | ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static Node *unary() {
    Token *tok;

    if ((tok = consume("sizeof", TK_KEYWORD))) {
        if (consume("(", TK_RESERVED)) {
            int sz;
            if (is_typename()) {
                Type *ty = abstract_declarator(declspec());
                sz = ty->size;
            } else {
                Node *node = expr();
                add_type(node);
                sz = node->ty->size;
            }

            expect(")");
            return new_node_num(sz, tok);
        }

        Node *node = unary();
        add_type(node);
        return new_node_num(node->ty->size, tok);
    }

    if ((tok = consume("+", TK_RESERVED))) {
        return new_node_binary(ND_ADD, new_node_num(0, NULL), cast(), tok);
    }

    if ((tok = consume("-", TK_RESERVED))) {
        return new_node_binary(ND_SUB, new_node_num(0, NULL), cast(), tok);
    }

    if ((tok = consume("*", TK_RESERVED))) {
        return new_node_unary(ND_DEREF, cast(), tok);
    }

    if ((tok = consume("&", TK_RESERVED))) {
        return new_node_unary(ND_ADDR, cast(), tok);
    }

    if ((tok = consume("~", TK_RESERVED))) {
        return new_node_unary(ND_BITNOT, cast(), tok);
    }

    if ((tok = consume("++", TK_RESERVED))) {
        Node *node = unary();
        return new_node_binary(ND_ASSIGN,
                               node,
                               new_node_add(node, new_node_num(1, NULL), NULL),
                               tok);
    }

    if ((tok = consume("--", TK_RESERVED))) {
        Node *node = unary();
        return new_node_binary(ND_ASSIGN,
                               node,
                               new_node_sub(node, new_node_num(1, NULL), NULL),
                               tok);
    }

    if ((tok = consume("!", TK_RESERVED))) {
        return new_node_unary(ND_NOT, cast(), tok);
    }

    return postfix();
}

static Node *ary_elem(Node *var, Node *idx) {
    return new_node_unary(ND_DEREF, new_node_add(var, idx, NULL), NULL);
}

static Member *struct_member(Type *ty) {
    for (Member *cur = ty->members; cur; cur = cur->next) {
        if (cur->name->len == token->len && !memcmp(cur->name->str, token->str, token->len)) {
            return cur;
        }
    }

    error_tok(token, "そのようなメンバは存在しません");
}

static Node *struct_ref(Node *lhs, Token *tok) {
    add_type(lhs);
    if (!is_type_of(TY_STRUCT, lhs->ty)) {
        error_tok(lhs->tok, "構造体ではありません");
    }

    Node *node = new_node_member(lhs, struct_member(lhs->ty), tok);
    return node;
}

// postfix = primary ("++" | "--" | (("." | "->") ident) | ("[" expr "]"))*
static Node *postfix() {
    Node *node = primary();
    Node *node_one = new_node_num(1, NULL);

    for (;;) {
        if (equal("++", TK_RESERVED)) {
            // `A++` は `(A += 1) - 1` と等価
            node = new_node_sub(to_assign(new_node_add(node, node_one, NULL)),
                                node_one,
                                consume("++", TK_RESERVED));

        } else if (equal("--", TK_RESERVED)) {
            // `A--` は `(A -= 1) + 1` と等価
            node = new_node_add(to_assign(new_node_sub(node, node_one, NULL)),
                                node_one,
                                consume("--", TK_RESERVED));

        } else if (equal(".", TK_RESERVED)) {
            node = struct_ref(node, consume(".", TK_RESERVED));
            token = token->next;

        } else if (equal("->", TK_RESERVED)) {
            node = struct_ref(new_node_unary(ND_DEREF, node, NULL),
                              consume("->", TK_RESERVED));
            token = token->next;

        } else if (consume("[", TK_RESERVED)) {
            node = ary_elem(node, expr());
            expect("]");
        } else {
            return node;
        }
    }
}

// primary = "(" (expr | ("{" compound-stmt)) ")"
//         | ident (("(" func-args? ")")
//         | ident
//         | str
//         | num
// func-args = assign ("," assign)*
static Node *primary() {
    if (consume("(", TK_RESERVED)) {
        Node *node;
        if (equal("{", TK_RESERVED)) {// GNU Statement Exprs
            node = new_node(ND_STMT_EXPR, consume("{", TK_RESERVED));
            node->body = compound_stmt()->body;
        } else {
            node = expr();
        }
        expect(")");
        return node;
    }

    // 識別子
    Token *tok = consume_ident();
    if (tok) {
        // 関数呼び出し
        if (consume("(", TK_RESERVED)) {
            if (!find_func(tok)) {
                error_tok(tok, "宣言されていない関数です");
            }

            Token *start = tok;

            Node head = {};
            Node *cur = &head;

            while (!consume(")", TK_RESERVED)) {
                if (consume(",", TK_RESERVED)) {
                    continue;
                }
                cur = cur->next = assign();
            }

            Node *node = new_node(ND_FUNCALL, tok);
            node->funcname = strndup(start->str, start->len);
            node->args = head.next;
            return node;
        }

        // 変数
        Var *var = find_var(tok);
        if (!var) {
            error_at(tok->str, "定義されていない変数です");
        }

        return new_node_var(var, tok);
    }

    // 文字列リテラル
    if (token->kind == TK_STR) {
        Var *var = new_str_literal(token, token->str);
        Node *node = new_node_var(var, token);
        token = token->next;
        return node;
    }

    // 数値
    tok = expect_number();
    return new_node_num(tok->val, tok);
}
