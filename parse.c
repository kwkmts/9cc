#include "9cc.h"

//
// パーサー
//

Token *token;
Obj *locals;
Obj *globals;
Obj *functions;
Obj *cur_fn;         // 現在パース中の関数
static Node *gotos;  // 現在の関数内におけるgoto文のリスト
static Node *labels; // 現在の関数内におけるラベルのリスト
static int cur_brk_label_id; // 現在のbreak文のジャンプ先ラベルID
static int cur_cont_label_id; // 現在のcontinue文のジャンプ先ラベルID
static Node *cur_switch;      // 現在パース中のswitch文ノード

// 列挙定数の型
typedef struct EnumConst EnumConst;
struct EnumConst {
    Token *name;
    int val;
    Type *ty;
};

typedef struct VarScope VarScope;
struct VarScope {
    VarScope *next;
    char *name;
    Obj *var;
    EnumConst *enum_const;
    Type *typedef_;
};

typedef struct TagScope TagScope;
struct TagScope {
    TagScope *next;
    Type *ty;
};

// ブロックスコープの型
typedef struct Scope Scope;
struct Scope {
    Scope *parent;  // 親のスコープ
    VarScope *vars; // スコープに属する変数
    TagScope *tags; // スコープに属するタグ
};

Scope *scope = &(Scope){};

// 初期化における指示子の型
typedef struct InitDesg InitDesg;
struct InitDesg {
    InitDesg *next;
    int idx;
    Member *member;
    Obj *var;
};

typedef int VarAttr;
#define NOATTR 0x00
#define TYPEDEF 0x01
#define STATIC 0x02
#define EXTERN 0x04

// 指定したトークンが期待しているトークンの時は真を返し、それ以外では偽を返す
static bool equal(char *op, TokenKind kind, Token *tok) {
    return tok->kind == kind && strlen(op) == tok->len &&
           !memcmp(tok->str, op, tok->len);
}

// 次のトークンが期待しているトークンの時は、トークンを1つ読み進めてそのトークンを返す
// それ以外の場合はNULLを返す
static Token *consume(char *op, TokenKind kind) {
    if (!equal(op, kind, token)) {
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
    if (!equal(op, TK_RESERVED, token)) {
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

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "識別子ではありません");
    }
    return strndup(tok->str, tok->len);
}

static bool at_eof() { return token->kind == TK_EOF; }

static void push_var_into_var_scope(char *name, Obj *var) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->var = var;
    sc->next = scope->vars;
    scope->vars = sc;
}

static void push_enum_const_into_var_scope(char *name, EnumConst *ec) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->enum_const = ec;
    sc->next = scope->vars;
    scope->vars = sc;
}

static void push_typedef_into_var_scope(char *name, Type *td) {
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->typedef_ = td;
    sc->next = scope->vars;
    scope->vars = sc;
}

static void push_tag_scope(Type *ty) {
    TagScope *sc = calloc(1, sizeof(TagScope));
    sc->ty = ty;
    sc->next = scope->tags;
    scope->tags = sc;
}

static void enter_scope() {
    Scope *sc = calloc(1, sizeof(Scope));
    sc->parent = scope;
    scope = sc;
}

static void leave_scope() { scope = scope->parent; }

int64_t calc_const_expr(Node *node) {
    switch (node->kind) {
    case ND_ADD:
        return calc_const_expr(node->binop.lhs) +
               calc_const_expr(node->binop.rhs);
    case ND_SUB:
        return calc_const_expr(node->binop.lhs) -
               calc_const_expr(node->binop.rhs);
    case ND_MUL:
        return calc_const_expr(node->binop.lhs) *
               calc_const_expr(node->binop.rhs);
    case ND_DIV:
        return calc_const_expr(node->binop.lhs) /
               calc_const_expr(node->binop.rhs);
    case ND_EQ:
        return calc_const_expr(node->binop.lhs) ==
               calc_const_expr(node->binop.rhs);
    case ND_NE:
        return calc_const_expr(node->binop.lhs) !=
               calc_const_expr(node->binop.rhs);
    case ND_LT:
        return calc_const_expr(node->binop.lhs) <
               calc_const_expr(node->binop.rhs);
    case ND_LE:
        return calc_const_expr(node->binop.lhs) <=
               calc_const_expr(node->binop.rhs);
    case ND_LOGAND:
        return calc_const_expr(node->binop.lhs) &&
               calc_const_expr(node->binop.rhs);
    case ND_LOGOR:
        return calc_const_expr(node->binop.lhs) ||
               calc_const_expr(node->binop.rhs);
    case ND_BITAND:
        return calc_const_expr(node->binop.lhs) &
               calc_const_expr(node->binop.rhs);
    case ND_BITOR:
        return calc_const_expr(node->binop.lhs) |
               calc_const_expr(node->binop.rhs);
    case ND_BITXOR:
        return calc_const_expr(node->binop.lhs) ^
               calc_const_expr(node->binop.rhs);
    case ND_SHL:
        return calc_const_expr(node->binop.lhs)
               << calc_const_expr(node->binop.rhs);
    case ND_SHR:
        return calc_const_expr(node->binop.lhs) >>
               calc_const_expr(node->binop.rhs);
    case ND_COMMA:
        return calc_const_expr(node->binop.rhs);
    case ND_COND:
        return calc_const_expr(node->condop.cond)
                   ? calc_const_expr(node->condop.then)
                   : calc_const_expr(node->condop.els);
    case ND_NOT:
        return !calc_const_expr(node->unary.expr);
    case ND_BITNOT:
        return ~calc_const_expr(node->unary.expr);
    case ND_CAST:
        if (is_integer(node->ty)) {
            switch (node->ty->size) {
            case 1:
                return (int8_t)calc_const_expr(node->unary.expr);
            case 2:
                return (int16_t)calc_const_expr(node->unary.expr);
            case 4:
                return (int32_t)calc_const_expr(node->unary.expr);
            }
        }
        return calc_const_expr(node->unary.expr);
    case ND_NUM:
        return node->num.val;
    default:
        error_tok(node->tok, "定数式ではありません");
    }
}

static VarScope *look_in_var_scope(Token *tok) {
    if (tok->kind != TK_IDENT) {
        return NULL;
    }
    for (Scope *sc = scope; sc; sc = sc->parent) {
        for (VarScope *var_sc = sc->vars; var_sc; var_sc = var_sc->next) {
            if (tok->len == (int)strlen(var_sc->name) &&
                !memcmp(tok->str, var_sc->name, (int)strlen(var_sc->name))) {
                return var_sc;
            }
        }
    }
    return NULL;
}

// 関数を名前で検索。見つからなければNULLを返す
static Obj *find_func(Token *tok) {
    VarScope *sc = look_in_var_scope(tok);
    if (!sc || !sc->var) {
        return NULL;
    }
    if (sc->var->kind == FUNC) {
        return sc->var;
    }
    return NULL;
}

// typedef宣言子を名前で検索。見つからなければNULLを返す
static Type *find_typedef(Token *tok) {
    VarScope *sc = look_in_var_scope(tok);
    if (sc) {
        return sc->typedef_;
    }
    return NULL;
}

// 構造体・共用体・列挙体タグを名前で検索。見つからなければNULLを返す
static Type *find_tag(Token *tok) {
    for (Scope *sc = scope; sc; sc = sc->parent) {
        for (TagScope *tag_sc = sc->tags; tag_sc; tag_sc = tag_sc->next) {
            if (tag_sc->ty->name->len == tok->len &&
                !memcmp(tok->str, tag_sc->ty->name->str,
                        tag_sc->ty->name->len)) {
                return tag_sc->ty;
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
    node->binop.lhs = lhs;
    node->binop.rhs = rhs;
    return node;
}

static Node *new_node_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->unary.expr = expr;
    return node;
}

static Node *new_node_num(int64_t val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->num.val = val;
    return node;
}

Node *new_node_cast(Node *expr, Type *ty) {
    add_type(expr);

    Node *node = new_node(ND_CAST, ty->ident);
    node->unary.expr = expr;
    node->ty = copy_type(ty);
    return node;
}

static Node *new_node_var(Obj *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->ty = var->ty;
    node->var.var = var;
    return node;
}

static Node *new_node_member(Node *lhs, Member *member, Token *tok) {
    Node *node = new_node(ND_MEMBER, tok);
    node->member.lhs = lhs;
    node->member.mem = member;
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
    rhs = new_node_binary(ND_MUL, rhs, new_node_num(lhs->ty->base->size, NULL),
                          NULL);
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
        rhs = new_node_binary(ND_MUL, rhs,
                              new_node_num(lhs->ty->base->size, NULL), NULL);
        return new_node_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - ptr (ポインタ間の要素数を返す)
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_node_binary(ND_SUB, lhs, rhs, tok);
        node->ty = ty_int;
        return new_node_binary(ND_DIV, node,
                               new_node_num(lhs->ty->base->size, NULL), NULL);
    }

    error_at(tok->str, "数値からポインタ値を引くことはできません");
}

static Obj *new_obj(char *name, Type *ty) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    push_var_into_var_scope(var->name, var);
    return var;
}

static Obj *new_lvar(char *name, Type *ty) {
    Obj *var = new_obj(name, ty);
    var->next = locals;
    var->offset =
        align_to(locals ? locals->offset + ty->size : ty->size, ty->align);
    var->kind = LVAR;
    locals = var;
    return var;
}

static Obj *new_gvar(char *name, Type *ty) {
    Obj *var = new_obj(name, ty);
    var->next = globals;
    var->kind = GVAR;
    var->has_definition = true;
    globals = var;
    return var;
}

static Obj *new_func(char *name, Type *ty) {
    Obj *fn = new_obj(name, ty);
    fn->kind = FUNC;
    fn->next = functions;
    fn->name = name;
    fn->ty = ty;
    fn->has_definition = false;
    functions = fn;
    return fn;
}

static char *to_init_data(char *str) {
    // e.g.) str: "\a\a\a"(長さ3)の場合、buf: "\\007\\007\\007"(長さ12)となる。
    //       strの長さの4倍 + '\0'分のメモリを確保
    char *buf = calloc(4 * strlen(str) + 1, 1);

    for (char *p = str; *p; p++) {
        switch (*p) {
        case '\a':
            strcat(buf, "\\007");
            break;
        case '\b':
            strcat(buf, "\\b");
            break;
        case '\t':
            strcat(buf, "\\t");
            break;
        case '\n':
            strcat(buf, "\\n");
            break;
        case '\v':
            strcat(buf, "\\013");
            break;
        case '\f':
            strcat(buf, "\\f");
            break;
        case '\r':
            strcat(buf, "\\r");
            break;
        case 27:
            strcat(buf, "\\033"); // GNU拡張(ASCII ESC)
            break;
        case '"':
            strcat(buf, "\\\"");
            break;
        case '\\':
            strcat(buf, "\\\\");
            break;
        case '\0':
            strcat(buf, "\\000");
            break;
        default:
            strncat(buf, p, 1);
            break;
        }
    }

    return buf;
}

static Obj *new_str_literal(Token *tok) {
    static int str_count = 0;
    Obj *var = new_gvar(format(".Lstr%d", str_count++),
                        array_of(ty_char, tok->len + 1));
    var->init_data_str = to_init_data(tok->str);
    return var;
}

// A op= B を `tmp = &A, *tmp = *tmp op B` に変換する
static Node *to_assign(Node *binary) {
    add_type(binary->binop.lhs);
    add_type(binary->binop.rhs);

    Node *var = new_node_var(
        new_lvar(strndup("", 0), pointer_to(binary->binop.lhs->ty)), NULL);

    Node *expr1 = new_node_binary(
        ND_ASSIGN, var, new_node_unary(ND_ADDR, binary->binop.lhs, NULL), NULL);

    Node *expr2 = new_node_binary(
        ND_ASSIGN, new_node_unary(ND_DEREF, var, NULL),
        new_node_binary(binary->kind, new_node_unary(ND_DEREF, var, NULL),
                        binary->binop.rhs, NULL),
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

    if (is_type_of(TY_STRUCT, ty) || is_type_of(TY_UNION, ty)) {
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

static void typedef_();
static Type *declspec();
static Type *declarator(Type *ty);
static Obj *function(Type *ty);
static Node *lvar_initializer(Obj *var);
static void gvar_initializer(Obj *var);
static Node *declaration(VarAttr attr);
static Node *stmt();
static Node *compound_stmt();
static Node *expr();
static Node *assign();
static Node *conditional();
static Node *logor();
static Node *logand();
static Node * bitor ();
static Node *bitxor();
static Node *bitand();
static Node *equality();
static Node *relational();
static Node *shift();
static Node *add();
static Node *mul();
static Node *cast();
static Node *unary();
static Node *postfix();
static Node *primary();

// program = (global-declaration | typedef)*
// global-declaration = declspec declarator function-decl
//                    | declspec declarator ("=" initializer)? ";"
//                    | declspec ";"
void program() {
    while (!at_eof()) {
        // typedef
        if (consume("typedef", TK_KEYWORD)) {
            typedef_();
            continue;
        }

        VarAttr attr = NOATTR;
        if (consume("static", TK_KEYWORD)) {
            attr |= STATIC;
        }
        if (consume("extern", TK_KEYWORD)) {
            attr |= EXTERN;
        }

        Type *basety = declspec();

        if (consume(";", TK_RESERVED)) {
            continue;
        }

        Type *ty = declarator(basety);

        // 関数定義
        if (is_type_of(TY_FUNC, ty)) {
            Obj *fn = function(ty);
            fn->is_static = attr & STATIC;
            continue;
        }

        // グローバル変数
        Obj *var = new_gvar(get_ident(ty->ident), ty);
        var->is_static = attr & STATIC;
        var->has_definition = !(attr & EXTERN);
        if (consume("=", TK_RESERVED)) {
            gvar_initializer(var);
        }
        expect(";");
    }
}

static void typedef_() {
    Type *ty = copy_type(declarator(declspec()));
    expect(";");

    ty->name = ty->ident;
    push_typedef_into_var_scope(get_ident(ty->name), ty);
}

// struct-decl = ident? "{" (declspec declarator ";")* "}"
//             | ident
static Type *struct_decl() {
    Token *tag = consume_ident();
    if (tag && !equal("{", TK_RESERVED, token)) {
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

    if (tag) {
        ty->name = tag;
        push_tag_scope(ty);
    }

    return ty;
}

// union-decl = ident? "{" (declspec declarator ";")* "}"
//             | ident
static Type *union_decl() {
    Token *tag = consume_ident();
    if (tag && !equal("{", TK_RESERVED, token)) {
        Type *ty = find_tag(tag);
        if (!ty) {
            error_tok(tag, "そのような共用体型はありません");
        }
        return ty;
    }

    expect("{");
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_UNION;
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
        mem->offset = 0;

        if (offset < ty2->size) {
            offset = ty2->size;
        }

        if (ty->align < mem->ty->align) {
            ty->align = mem->ty->align;
        }

        cur = cur->next = mem;
    }

    ty->members = head.next;
    ty->size = align_to(offset, ty->align);

    if (tag) {
        ty->name = tag;
        push_tag_scope(ty);
    }

    return ty;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident
// enum-list = ident ("=" num)? ("," ident ("=" num)?)*
static Type *enum_specifier() {
    Type *ty = enum_type();
    ty->name = consume_ident();
    if (ty->name && !equal("{", TK_RESERVED, token)) {
        Type *ty2 = find_tag(ty->name);
        if (!ty2) {
            error_tok(ty->name, "そのような列挙型はありません");
        }
        return ty2;
    }

    expect("{");
    int val = 0;
    for (int i = 0; !consume("}", TK_RESERVED); i++) {
        if (i > 0) {
            expect(",");
        }

        Token *tok = expect_ident();
        // TODO: 重複チェック

        if (consume("=", TK_RESERVED)) {
            val = (int)expect_number()->val;
        }

        EnumConst *ec = calloc(1, sizeof(EnumConst));
        ec->name = tok;
        ec->val = val++;
        ec->ty = ty;
        push_enum_const_into_var_scope(get_ident(tok), ec);
    }

    if (ty->name) {
        push_tag_scope(ty);
    }

    return ty;
}

// declspec = "void" | "int" | "char" | "short" | "long" |
//          | struct-decl | union-decl | enum-specifier | typedef-name
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

    if (consume("union", TK_KEYWORD)) {
        return union_decl();
    }

    if (consume("enum", TK_KEYWORD)) {
        return enum_specifier();
    }

    Type *ty = find_typedef(token);
    token = token->next;
    return ty;
}

// ary-suffix = "[" num? "]" ary-suffix?
static Type *ary_suffix(Type *ty) {
    if (consume("[", TK_RESERVED)) {
        int size = token->kind == TK_NUM ? (int)expect_number()->val : -1;
        expect("]");
        ty = ary_suffix(ty);
        return array_of(ty, size);
    }

    return ty;
}

// func-suffix = "(" func-params? ")"
// func-params = declspec declarator ("," declspec declarator)*
static Type *func_suffix(Type *ty) {
    expect("(");

    Type head = {};
    Type *cur = &head;

    while (!consume(")", TK_RESERVED)) {
        if (cur != &head) {
            expect(",");
        }

        cur = cur->next = copy_type(declarator(declspec()));
    }

    return func_type(ty, head.next);
}

// type-suffix = ary-suffix | func-suffix | ε
static Type *type_suffix(Type *ty) {
    if (equal("[", TK_RESERVED, token)) {
        return ary_suffix(ty);
    }

    if (equal("(", TK_RESERVED, token)) {
        return func_suffix(ty);
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
    // TODO: 変数の重複定義をエラーにする
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
    for (Node *x = gotos; x; x = x->goto_.next) {
        for (Node *y = labels; y; y = y->label.next) {
            if (strcmp(x->goto_.label, y->label.name) == 0) {
                x->goto_.id = y->label.id;
                break;
            }
        }

        if (!x->goto_.id) {
            error_tok(x->tok->next, "そのようなラベルはありません");
        }
    }
}

// function-decl = ("{" compound-stmt) | ";"
static Obj *function(Type *ty) {
    Obj *fn = find_func(ty->ident);
    if (!fn) {
        fn = new_func(get_ident(ty->ident), ty);
    }

    if (consume(";", TK_RESERVED)) {
        return fn;
    }

    cur_fn = fn;
    locals = NULL;
    enter_scope();
    for (Type *param = ty->params; param; param = param->next) {
        new_lvar(get_ident(param->ident), param);
    }

    fn->params = locals;
    expect("{");
    fn->body = compound_stmt();
    fn->locals = locals;
    fn->has_definition = true;
    leave_scope();
    resolve_goto_labels();

    return fn;
}

static void initializer2(Initializer *init);

static int count_ary_init_elems(Type *ty) {
    Token *tmp = token; // 要素数を数える直前のトークンのアドレスを退避
    Initializer *dummy = new_initializer(ty->base, false);
    int i = 0;
    while (!equal("}", TK_RESERVED, token)) {
        if (i > 0) {
            expect(",");
        }
        initializer2(dummy);
        i++;
    }

    token = tmp; // 現在着目しているトークンを元に戻す
    return i;
}

static void initialize_with_zero(Initializer *init) {
    if (is_type_of(TY_ARY, init->ty)) {
        for (int i = 0; i < init->ty->ary_len; i++) {
            initialize_with_zero(init->children[i]);
        }
        return;
    }

    if (is_type_of(TY_STRUCT, init->ty) || is_type_of(TY_UNION, init->ty)) {
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            initialize_with_zero(init->children[mem->idx]);
        }
        return;
    }

    init->expr = new_node_num(0, NULL);
}

// ary-initializer = "{" (initializer ("," initializer)*)? "}"}
//                 | ε
static void ary_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        return;
    }

    if (init->is_flexible) {
        *init = *new_initializer(
            array_of(init->ty->base, count_ary_init_elems(init->ty)), false);
    }

    int i = 0;
    while (i < init->ty->ary_len && !equal("}", TK_RESERVED, token)) {
        if (i > 0) {
            expect(",");
        }

        initializer2(init->children[i++]);
    }

    expect("}");
}

// struct-initializer = "{" (initializer ("," initializer)*)? "}"
//                    | assign
static void struct_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        Node *expr = assign();
        add_type(expr);
        if (is_type_of(TY_STRUCT, expr->ty)) {
            init->expr = expr;
            return;
        }
    }

    Member *mem = init->ty->members;
    while (mem && !equal("}", TK_RESERVED, token)) {
        if (mem != init->ty->members) {
            expect(",");
        }

        initializer2(init->children[mem->idx]);
        mem = mem->next;
    }

    expect("}");
}

// union-initializer = "{" initializer "}"
//                   | assign
static void union_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        Node *expr = assign();
        add_type(expr);
        if (is_type_of(TY_UNION, expr->ty)) {
            init->expr = expr;
            return;
        }
    }

    initializer2(init->children[0]);
    expect("}");
}

// str-initializer = str
static void str_initializer(Initializer *init) {
    if (init->is_flexible) {
        *init =
            *new_initializer(array_of(init->ty->base, token->len + 1), false);
        initialize_with_zero(init);
    }

    int len =
        init->ty->ary_len < token->len + 1 ? init->ty->ary_len : token->len + 1;
    for (int i = 0; i < len; i++) {
        init->children[i]->expr = new_node_num(token->str[i], token);
    }

    token = token->next;
}

// initializer = str-initializer | ary-initializer
//             | struct-initializer | union-initializer | assign
static void initializer2(Initializer *init) {
    if (is_type_of(TY_ARY, init->ty) && token->kind == TK_STR) {
        str_initializer(init);
        return;
    }

    if (is_type_of(TY_ARY, init->ty)) {
        ary_initializer(init);
        return;
    }

    if (is_type_of(TY_STRUCT, init->ty)) {
        struct_initializer(init);
        return;
    }

    if (is_type_of(TY_UNION, init->ty)) {
        union_initializer(init);
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

// 配列の各要素や構造体の各メンバを参照するノードを返す
static Node *init_target_elem_mem(InitDesg *desg) {
    if (desg->var) {
        return new_node_var(desg->var, NULL);
    }

    if (desg->member) {
        return new_node_member(init_target_elem_mem(desg->next), desg->member,
                               NULL);
    }

    return new_node_unary(ND_DEREF,
                          new_node_add(init_target_elem_mem(desg->next),
                                       new_node_num(desg->idx, NULL), NULL),
                          NULL);
}

// 配列の各要素や構造体の各メンバに対して初期化が適用されるように木構造(ND_ASSIGNをND_COMMAでつなげたもの)を返す
static Node *create_lvar_init(Initializer *init, InitDesg *desg) {
    if (is_type_of(TY_ARY, init->ty)) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (int i = 0; i < init->ty->ary_len; i++) {
            InitDesg desg2 = {desg, i};
            node = new_node_binary(ND_COMMA, node,
                                   create_lvar_init(init->children[i], &desg2),
                                   NULL);
        }

        return node;
    }

    if (is_type_of(TY_STRUCT, init->ty) && !init->expr) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            InitDesg desg2 = {desg, 0, mem};
            node = new_node_binary(
                ND_COMMA, node,
                create_lvar_init(init->children[mem->idx], &desg2), NULL);
        }

        return node;
    }

    if (is_type_of(TY_UNION, init->ty) && !init->expr) {
        InitDesg desg2 = {desg, 0, init->ty->members};
        return new_node_binary(ND_COMMA, new_node(ND_NULL_EXPR, NULL),
                               create_lvar_init(init->children[0], &desg2),
                               NULL);
    }

    return new_node_binary(ND_ASSIGN, init_target_elem_mem(desg), init->expr,
                           NULL);
}

static Node *lvar_initializer(Obj *var) {
    Initializer *init = initializer(var->ty, &var->ty);
    // 配列の要素数が省略された場合、var->ty->sizeがこの時点で確定するのでvar->offsetを更新
    var->offset =
        locals->next ? locals->next->offset + var->ty->size : var->ty->size;
    InitDesg desg = {NULL, 0, NULL, var};
    return create_lvar_init(init, &desg);
}

static void gvar_initializer(Obj *var) {
    var->init = initializer(var->ty, &var->ty);
}

// declaration = declspec declarator ("=" initializer)? ";"
//             | declspec ";"
static Node *declaration(VarAttr attr) {
    Node head = {};
    Node *cur = &head;

    Type *basety = declspec();

    if (!equal(";", TK_RESERVED, token)) {
        Type *ty = declarator(basety);
        if (is_type_of(TY_VOID, ty) && !is_type_of(TY_FUNC, ty)) {
            error_tok(ty->ident, "void型の変数を宣言することはできません");
        }

        if (attr & STATIC) {
            static int static_count = 0;
            char *name = get_ident(ty->ident);
            char *unique_name =
                format("%s.%s.%d", cur_fn->name, name, static_count++);
            Obj *var = new_gvar(unique_name, ty);
            push_var_into_var_scope(name, var);
            if (consume("=", TK_RESERVED)) {
                gvar_initializer(var);
            }
        } else {
            Obj *var = new_lvar(get_ident(ty->ident), ty);

            if (consume("=", TK_RESERVED)) {
                Node *node = new_node(ND_INIT, NULL);
                node->init.assigns = lvar_initializer(var);
                cur->next = node;
            }

            if (var->ty->ary_len < 0) {
                error_at(ty->ident->str, "配列の要素数が指定されていません");
            }
        }
    }

    expect(";");

    Node *node = new_node(ND_BLOCK, NULL);
    node->block.body = head.next;
    return node;
}

static bool is_typename() {
    static char *kw[] = {"void", "int",    "char",  "short",
                         "long", "struct", "union", "enum"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(kw[i], TK_KEYWORD, token)) {
            return true;
        }
    }
    return find_typedef(token);
}

// expr-stmt = expr ";"
static Node *expr_stmt() {
    Node *lhs = expr();
    Node *node = new_node(ND_EXPR_STMT, expect(";"));
    node->exprstmt.expr = lhs;
    return node;
}

// if-stmt = "if" "(" expr ")" stmt ("else" stmt)?
static Node *if_stmt() {
    Node *node = new_node(ND_IF, consume("if", TK_KEYWORD));

    if (consume("(", TK_RESERVED)) {
        node->if_.cond = expr();
        expect(")");
    }

    node->if_.then = stmt();

    if (consume("else", TK_KEYWORD)) {
        node->if_.els = stmt();
    }

    return node;
}

// switch-stmt = "switch" "(" expr ")" stmt
static Node *switch_stmt() {
    Node *node = new_node(ND_SWITCH, consume("switch", TK_KEYWORD));
    expect("(");
    node->switch_.cond = expr();
    expect(")");

    Node *sw = cur_switch;
    cur_switch = node;

    int brk_label_id = cur_brk_label_id;
    node->switch_.brk_label_id = cur_brk_label_id = count();
    node->switch_.then = stmt();

    cur_switch = sw;
    cur_brk_label_id = brk_label_id;

    return node;
}

// case-stmt = "case" num ":" stmt
static Node *case_stmt() {
    Token *tok = consume("case", TK_KEYWORD);
    int64_t val = expect_number()->val;

    if (!cur_switch) {
        error_tok(tok, "ここでcase文を使用することはできません");
    }

    for (Node *n = cur_switch->switch_.case_next; n; n = n->case_.next) {
        if (val == n->case_.val) {
            error_tok(tok, "重複したcase文");
        }
    }

    expect(":");

    Node *node = new_node(ND_CASE, tok);
    node->case_.val = val;
    node->case_.id = count();
    node->case_.stmt = stmt();
    node->case_.next = cur_switch->switch_.cases;
    cur_switch->switch_.cases = node;
    return node;
}

// default-stmt = "default" ":" stmt
static Node *default_stmt() {
    Token *tok = consume("default", TK_KEYWORD);
    if (!cur_switch) {
        error_tok(tok, "ここでdefault文を使用することはできません");
    }
    if (cur_switch->switch_.default_) {
        error_tok(tok, "重複したdefault文");
    }

    expect(":");

    Node *node = new_node(ND_CASE, tok);
    node->case_.id = count();
    node->case_.stmt = stmt();
    cur_switch->switch_.default_ = node;
    return node;
}

// while-stmt = "while" "(" expr ")" stmt
static Node *while_stmt() {
    Node *node = new_node(ND_WHILE, consume("while", TK_KEYWORD));

    if (consume("(", TK_RESERVED)) {
        node->while_.cond = expr();
        expect(")");
    }

    int brk_label_id = cur_brk_label_id;
    int cont_label_id = cur_cont_label_id;
    node->while_.brk_label_id = cur_brk_label_id = count();
    node->while_.cont_label_id = cur_cont_label_id = count();
    node->while_.then = stmt();
    cur_brk_label_id = brk_label_id;
    cur_cont_label_id = cont_label_id;
    return node;
}

// for-stmt = "for" "(" expr? ";" expr? ";" expr? ")" stmt
static Node *for_stmt() {
    Node *node = new_node(ND_FOR, consume("for", TK_KEYWORD));
    expect("(");

    enter_scope();

    if (!consume(";", TK_RESERVED)) {
        node->for_.init = is_typename() ? declaration(NOATTR) : expr_stmt();
    }

    if (!consume(";", TK_RESERVED)) {
        node->for_.cond = expr();
        expect(";");
    }

    if (!consume(")", TK_RESERVED)) {
        node->for_.after = expr();
        expect(")");
    }

    int brk_label_id = cur_brk_label_id;
    int cont_label_id = cur_cont_label_id;
    node->for_.brk_label_id = cur_brk_label_id = count();
    node->for_.cont_label_id = cur_cont_label_id = count();
    node->for_.then = stmt();

    leave_scope();

    cur_brk_label_id = brk_label_id;
    cur_cont_label_id = cont_label_id;

    return node;
}

// goto-stmt = "goto" ident ";"
static Node *goto_stmt() {
    Node *node = new_node(ND_GOTO, consume("goto", TK_KEYWORD));
    Token *tok = expect_ident();
    node->goto_.label = get_ident(tok);
    node->goto_.next = gotos;
    gotos = node;
    expect(";");
    return node;
}

// label-stmt = ident ":" stmt
static Node *label_stmt() {
    Token *name = expect_ident();
    expect(":");
    Node *node = new_node(ND_LABEL, name);
    node->label.name = get_ident(name);
    node->label.id = count();
    node->label.stmt = stmt();
    node->label.next = labels;
    labels = node;
    return node;
}

// break-stmt = "break" ";"
static Node *break_stmt() {
    Token *tok = consume("break", TK_KEYWORD);
    if (!cur_brk_label_id) {
        error_tok(tok, "ここでbreak文を使用することはできません");
    }
    Node *node = new_node(ND_GOTO, tok);
    node->goto_.id = cur_brk_label_id;
    expect(";");
    return node;
}

// continue-stmt = "continue" ";"
static Node *continue_stmt() {
    Token *tok = consume("continue", TK_KEYWORD);
    if (!cur_cont_label_id) {
        error_tok(tok, "ここでcontinue文を使用することはできません");
    }
    Node *node = new_node(ND_GOTO, tok);
    node->goto_.id = cur_cont_label_id;
    expect(";");
    return node;
}

// return-stmt = "return" expr ";"
static Node *return_stmt() {
    Node *node = new_node(ND_RETURN, consume("return", TK_KEYWORD));
    Node *exp = node->return_.expr = expr();
    expect(";");

    add_type(exp);
    node->return_.expr = new_node_cast(exp, cur_fn->ty->ret);
    return node;
}

// stmt = expr-stmt
//      | ";"
//      | "{" compound-stmt
//      | if-stmt
//      | switch-stmt
//      | case-stmt
//      | default-stmt
//      | while-stmt
//      | for-stmt
//      | goto-stmt
//      | label-stmt
//      | break-stmt
//      | continue-stmt
//      | return-stmt
static Node *stmt() {
    if (consume("{", TK_RESERVED)) {
        return compound_stmt();
    }
    if (equal("if", TK_KEYWORD, token)) {
        return if_stmt();
    }
    if (equal("switch", TK_KEYWORD, token)) {
        return switch_stmt();
    }
    if (equal("case", TK_KEYWORD, token)) {
        return case_stmt();
    }
    if (equal("default", TK_KEYWORD, token)) {
        return default_stmt();
    }
    if (equal("while", TK_KEYWORD, token)) {
        return while_stmt();
    }
    if (equal("for", TK_KEYWORD, token)) {
        return for_stmt();
    }
    if (equal("goto", TK_KEYWORD, token)) {
        return goto_stmt();
    }
    if (equal("break", TK_KEYWORD, token)) {
        return break_stmt();
    }
    if (equal("continue", TK_KEYWORD, token)) {
        return continue_stmt();
    }
    if (equal("return", TK_KEYWORD, token)) {
        return return_stmt();
    }
    if (equal(";", TK_RESERVED, token)) {
        return new_node(ND_NULL_STMT, consume(";", TK_RESERVED));
    }
    if (token->kind == TK_IDENT && equal(":", TK_RESERVED, token->next)) {
        return label_stmt();
    }

    return expr_stmt();
}

// compound-stmt = (stmt | declaration | typedef)* "}"
static Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK, NULL);
    Node head = {};

    enter_scope();

    for (Node *cur = &head; !consume("}", TK_RESERVED);) {
        if (consume("typedef", TK_KEYWORD)) {
            typedef_();
            continue;
        }

        VarAttr attr = NOATTR;
        if (consume("static", TK_KEYWORD)) {
            attr |= STATIC;
        }
        if (consume("extern", TK_KEYWORD)) {
            attr |= EXTERN;
        }

        if (is_typename() && !equal(":", TK_RESERVED, token->next)) {
            cur = cur->next = declaration(attr);
        } else {
            cur = cur->next = stmt();
        }
        add_type(cur);
    }

    leave_scope();

    node->block.body = head.next;

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

// assign = conditional (("=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" |
// "^=" | "<<=" | ">>=") assign)?
static Node *assign() {
    Node *node = conditional();
    Token *tok;

    if ((tok = consume("=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node, assign(), tok);
    }

    if ((tok = consume("+=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node,
                               new_node_add(node, assign(), NULL), tok);
    }

    if ((tok = consume("-=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node,
                               new_node_sub(node, assign(), NULL), tok);
    }

    if ((tok = consume("*=", TK_RESERVED))) {
        node =
            new_node_binary(ND_ASSIGN, node,
                            new_node_binary(ND_MUL, node, assign(), NULL), tok);
    }

    if ((tok = consume("/=", TK_RESERVED))) {
        node =
            new_node_binary(ND_ASSIGN, node,
                            new_node_binary(ND_DIV, node, assign(), NULL), tok);
    }

    if ((tok = consume("%=", TK_RESERVED))) {
        node =
            new_node_binary(ND_ASSIGN, node,
                            new_node_binary(ND_MOD, node, assign(), NULL), tok);
    }

    if ((tok = consume("&=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node,
                               new_node_binary(ND_BITAND, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("|=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node,
                               new_node_binary(ND_BITOR, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("^=", TK_RESERVED))) {
        node = new_node_binary(ND_ASSIGN, node,
                               new_node_binary(ND_BITXOR, node, assign(), NULL),
                               tok);
    }

    if ((tok = consume("<<=", TK_RESERVED))) {
        node =
            new_node_binary(ND_ASSIGN, node,
                            new_node_binary(ND_SHL, node, assign(), NULL), tok);
    }

    if ((tok = consume(">>=", TK_RESERVED))) {
        node =
            new_node_binary(ND_ASSIGN, node,
                            new_node_binary(ND_SHR, node, assign(), NULL), tok);
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
    node->condop.cond = cond;
    node->condop.then = expr();
    expect(":");
    node->condop.els = conditional();
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
    Node *node = bitor ();
    Token *tok;

    for (;;) {
        if ((tok = consume("&&", TK_RESERVED))) {
            node = new_node_binary(ND_LOGAND, node, bitor (), tok);
        } else {
            return node;
        }
    }
}

// bitor = bitxor ("|" bitxor)*
static Node * bitor () {
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

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node *relational() {
    Node *node = shift();
    Token *tok;

    for (;;) {
        if ((tok = consume("<", TK_RESERVED))) {
            node = new_node_binary(ND_LT, node, shift(), tok);

        } else if ((tok = consume("<=", TK_RESERVED))) {
            node = new_node_binary(ND_LE, node, shift(), tok);

        } else if ((tok = consume(">", TK_RESERVED))) {
            node = new_node_binary(ND_LT, shift(), node, tok);

        } else if ((tok = consume(">=", TK_RESERVED))) {
            node = new_node_binary(ND_LE, shift(), node, tok);

        } else {
            return node;
        }
    }
}

// shift = add ("<<" add | ">>" add)*
static Node *shift() {
    Node *node = add();
    Token *tok;

    for (;;) {
        if ((tok = consume("<<", TK_RESERVED))) {
            node = new_node_binary(ND_SHL, node, add(), tok);

        } else if ((tok = consume(">>", TK_RESERVED))) {
            node = new_node_binary(ND_SHR, node, add(), tok);

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
        return new_node_binary(ND_ASSIGN, node,
                               new_node_add(node, new_node_num(1, NULL), NULL),
                               tok);
    }

    if ((tok = consume("--", TK_RESERVED))) {
        Node *node = unary();
        return new_node_binary(ND_ASSIGN, node,
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
        if (cur->name->len == token->len &&
            !memcmp(cur->name->str, token->str, token->len)) {
            return cur;
        }
    }

    error_tok(token, "そのようなメンバは存在しません");
}

static Node *struct_ref(Node *lhs, Token *tok) {
    add_type(lhs);
    if (!is_type_of(TY_STRUCT, lhs->ty) && !is_type_of(TY_UNION, lhs->ty)) {
        error_tok(lhs->tok, "構造体/共用体ではありません");
    }

    Node *node = new_node_member(lhs, struct_member(lhs->ty), tok);
    return node;
}

// postfix = primary ("++" | "--" | (("." | "->") ident) | ("[" expr "]"))*
static Node *postfix() {
    Node *node = primary();
    Node *node_one = new_node_num(1, NULL);

    for (;;) {
        if (equal("++", TK_RESERVED, token)) {
            // `A++` は `(A += 1) - 1` と等価
            add_type(node);
            node = new_node_cast(
                new_node_sub(to_assign(new_node_add(node, node_one, NULL)),
                             node_one, consume("++", TK_RESERVED)),
                node->ty);

        } else if (equal("--", TK_RESERVED, token)) {
            // `A--` は `(A -= 1) + 1` と等価
            add_type(node);
            node = new_node_cast(
                new_node_add(to_assign(new_node_sub(node, node_one, NULL)),
                             node_one, consume("--", TK_RESERVED)),
                node->ty);

        } else if (equal(".", TK_RESERVED, token)) {
            node = struct_ref(node, consume(".", TK_RESERVED));
            token = token->next;

        } else if (equal("->", TK_RESERVED, token)) {
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

// funcall = ident "(" func-args? ")"
// func-args = assign ("," assign)*
static Node *funcall(Token *tok) {
    Obj *fn = find_func(tok);
    if (!fn) {
        error_tok(tok, "そのような関数は存在しません");
    }

    expect("(");

    Token *start = tok;

    Node head = {};
    Node *cur = &head;
    Type *param_ty = fn->ty->params;

    while (!consume(")", TK_RESERVED)) {
        if (cur != &head) {
            expect(",");
        }
        cur = cur->next = new_node_cast(assign(), fn->ty->params);
        param_ty = param_ty->next;
    }

    Node *node = new_node(ND_FUNCALL, tok);
    node->ty = fn->ty->ret;
    node->funcall.fn = fn;
    node->funcall.args = head.next;
    return node;
}

// primary = "(" (expr | ("{" compound-stmt)) ")"
//         | funcall
//         | ident
//         | str
//         | num
static Node *primary() {
    if (consume("(", TK_RESERVED)) {
        Node *node;
        if (equal("{", TK_RESERVED, token)) { // GNU Statement Exprs
            node = new_node(ND_STMT_EXPR, consume("{", TK_RESERVED));
            node->block.body = compound_stmt()->block.body;
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
        if (equal("(", TK_RESERVED, token)) {
            return funcall(tok);
        }

        // 変数または列挙定数
        VarScope *sc = look_in_var_scope(tok);
        if (!sc || !sc->var && !sc->enum_const) {
            error_at(tok->str, "定義されていない変数です");
        }

        if (sc->var) {
            return new_node_var(sc->var, tok);
        } else {
            return new_node_num(sc->enum_const->val, tok);
        }
    }

    // 文字列リテラル
    if (token->kind == TK_STR) {
        Obj *var = new_str_literal(token);
        Node *node = new_node_var(var, token);
        token = token->next;
        return node;
    }

    // 数値
    tok = expect_number();
    return new_node_num(tok->val, tok);
}
