#include "9cc.h"

//
// パーサー
//

static Token *token; // 現在着目しているトークン
void set_token_to_parse(Token *tok) { token = tok; }

Obj locals = (Obj){};
Obj globals = (Obj){};
Obj functions = (Obj){};
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

typedef struct {
    int storage; // 記憶クラス指定子のフラグ
    int align;   // _Alignas指定子の値
} VarAttr;
#define TYPEDEF 0x01
#define STATIC 0x04
#define EXTERN 0x10

// 指定したトークンが期待しているトークンの時は真を返し、それ以外では偽を返す
bool equal(char *op, TokenKind kind, Token *tok) {
    return tok->kind == kind && strlen(op) == tok->len &&
           !memcmp(tok->loc, op, tok->len);
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

static bool is_list_end() {
    return equal("}", TK_RESERVED, token) ||
           equal(",", TK_RESERVED, token) &&
               equal("}", TK_RESERVED, token->next);
}

static bool consume_list_end() {
    if (consume("}", TK_RESERVED)) {
        return true;
    }

    if (equal(",", TK_RESERVED, token) &&
        equal("}", TK_RESERVED, token->next)) {
        token = token->next->next;
        return true;
    }

    return false;
}

// 次のトークンが期待している記号の時は、トークンを1つ読み進めてその記号を返す
// それ以外の場合にはエラーを報告
static Token *expect(char *op) {
    if (!equal(op, TK_RESERVED, token)) {
        error_tok(token, "'%s'ではありません", op);
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す
// それ以外の場合にはエラーを報告
static Token *expect_number() {
    if (token->kind != TK_NUM) {
        error_tok(token, "数ではありません");
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
        error_tok(token, "識別子ではありません");
    }
    return ident;
}

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "識別子ではありません");
    }
    return get_str(tok);
}

bool at_eof(Token *tok) { return tok->kind == TK_EOF; }

static VarScope *look_in_cur_var_scope(Token *tok);

// 名前の重複がある場合はエラーを報告
static VarScope *new_var_scope(Token *tok) {
    char *name = get_ident(tok);
    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->next = scope->vars;
    scope->vars = sc;
    return sc;
}

static VarScope *push_var_into_var_scope(Token *tok, Obj *var) {
    VarScope *sc = look_in_cur_var_scope(tok);
    if (sc && sc->var && sc->var->has_definition) {
        error_tok(tok, "そのような識別子はすでに存在します");
    }
    sc = new_var_scope(tok);
    sc->var = var;
    return sc;
}

static void push_builtin_obj_into_var_scope() {
    char *builtins[] = {"__builtin_va_start", "__builtin_va_arg",
                        "__builtin_va_end", "__builtin_va_copy"};
    for (int i = 0; i < sizeof(builtins) / sizeof(*builtins); i++) {
        Obj *var = calloc(1, sizeof(Obj));
        var->kind = FUNC;
        var->name = builtins[i];

        VarScope *sc = calloc(1, sizeof(VarScope));
        sc->name = var->name;
        sc->next = scope->vars;
        scope->vars = sc;
        sc->var = var;
    }
}

static VarScope *push_enum_const_into_var_scope(Token *tok, EnumConst *ec) {
    if (look_in_cur_var_scope(tok)) {
        error_tok(tok, "そのような識別子はすでに存在します");
    }
    VarScope *sc = new_var_scope(tok);
    sc->enum_const = ec;
    return sc;
}

static VarScope *push_typedef_into_var_scope(Token *tok, Type *td) {
    if (look_in_cur_var_scope(tok)) {
        error_tok(tok, "そのような識別子はすでに存在します");
    }
    VarScope *sc = new_var_scope(tok);
    sc->typedef_ = td;
    return sc;
}

static void push_tag_scope(Type *ty) {
    assert(ty->name);
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

static int64_t calc_const_expr2(Node *node, char **label) {
    switch (node->kind) {
    case ND_VAR:
        *label = node->var.var->name;
        return 0;
    case ND_DEREF:
        return calc_const_expr2(node->unary.expr, label);
    default:
        unreachable();
    }
}

int64_t calc_const_expr(Node *node, char **label) {
    switch (node->kind) {
    case ND_ADD:
        return calc_const_expr(node->binop.lhs, label) +
               calc_const_expr(node->binop.rhs, label);
    case ND_SUB:
        return calc_const_expr(node->binop.lhs, label) -
               calc_const_expr(node->binop.rhs, label);
    case ND_MUL:
        return calc_const_expr(node->binop.lhs, label) *
               calc_const_expr(node->binop.rhs, label);
    case ND_DIV:
        return calc_const_expr(node->binop.lhs, label) /
               calc_const_expr(node->binop.rhs, label);
    case ND_EQ:
        return calc_const_expr(node->binop.lhs, label) ==
               calc_const_expr(node->binop.rhs, label);
    case ND_NE:
        return calc_const_expr(node->binop.lhs, label) !=
               calc_const_expr(node->binop.rhs, label);
    case ND_LT:
        return calc_const_expr(node->binop.lhs, label) <
               calc_const_expr(node->binop.rhs, label);
    case ND_LE:
        return calc_const_expr(node->binop.lhs, label) <=
               calc_const_expr(node->binop.rhs, label);
    case ND_LOGAND:
        return calc_const_expr(node->binop.lhs, label) &&
               calc_const_expr(node->binop.rhs, label);
    case ND_LOGOR:
        return calc_const_expr(node->binop.lhs, label) ||
               calc_const_expr(node->binop.rhs, label);
    case ND_BITAND:
        return calc_const_expr(node->binop.lhs, label) &
               calc_const_expr(node->binop.rhs, label);
    case ND_BITOR:
        return calc_const_expr(node->binop.lhs, label) |
               calc_const_expr(node->binop.rhs, label);
    case ND_BITXOR:
        return calc_const_expr(node->binop.lhs, label) ^
               calc_const_expr(node->binop.rhs, label);
    case ND_SHL:
        return calc_const_expr(node->binop.lhs, label)
               << calc_const_expr(node->binop.rhs, label);
    case ND_SHR:
        return calc_const_expr(node->binop.lhs, label) >>
               calc_const_expr(node->binop.rhs, label);
    case ND_COMMA:
        return calc_const_expr(node->binop.rhs, label);
    case ND_COND:
        return calc_const_expr(node->condop.cond, label)
                   ? calc_const_expr(node->condop.then, label)
                   : calc_const_expr(node->condop.els, label);
    case ND_NOT:
        return !calc_const_expr(node->unary.expr, label);
    case ND_BITNOT:
        return ~calc_const_expr(node->unary.expr, label);
    case ND_CAST:
        if (is_integer(node->ty)) {
            switch (node->ty->size) {
            case 1:
                return (int8_t)calc_const_expr(node->unary.expr, label);
            case 2:
                return (int16_t)calc_const_expr(node->unary.expr, label);
            case 4:
                return (int32_t)calc_const_expr(node->unary.expr, label);
            }
        }
        return calc_const_expr(node->unary.expr, label);
    case ND_NUM:
        return node->num.ival;
    case ND_ADDR:
        return calc_const_expr2(node->unary.expr, label);
    case ND_DEREF:
        if (node->unary.expr->ty->kind == TY_FUNC) {
            return calc_const_expr2(node, label);
        }
    case ND_VAR:
        if (node->var.var->ty->kind == TY_FUNC) {
            return calc_const_expr2(node, label);
        }
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
                !memcmp(tok->loc, var_sc->name, (int)strlen(var_sc->name))) {
                return var_sc;
            }
        }
    }
    return NULL;
}

static VarScope *look_in_cur_var_scope(Token *tok) {
    if (tok->kind != TK_IDENT) {
        return NULL;
    }
    for (VarScope *var_sc = scope->vars; var_sc; var_sc = var_sc->next) {
        if (tok->len == (int)strlen(var_sc->name) &&
            !memcmp(tok->loc, var_sc->name, (int)strlen(var_sc->name))) {
            return var_sc;
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
static Type *find_tag(Token *tok, bool *in_cur_scope) {
    Scope *cur_sc = scope;
    *in_cur_scope = false;

    for (Scope *sc = cur_sc; sc; sc = sc->parent) {
        for (TagScope *tag_sc = sc->tags; tag_sc; tag_sc = tag_sc->next) {
            if (tag_sc->ty->name->len == tok->len &&
                !memcmp(tok->loc, tag_sc->ty->name->loc,
                        tag_sc->ty->name->len)) {
                *in_cur_scope = (sc == cur_sc);
                return tag_sc->ty;
            }
        }
    }
    return NULL;
}

static Type *find_tag_in_cur_scope(Token *tok) {
    for (TagScope *tag_sc = scope->tags; tag_sc; tag_sc = tag_sc->next) {
        if (tag_sc->ty->name->len == tok->len &&
            !memcmp(tok->loc, tag_sc->ty->name->loc, tag_sc->ty->name->len)) {
            return tag_sc->ty;
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

Node *new_node_unary(NodeKind kind, Node *expr, Token *tok) {
    Node *node = new_node(kind, tok);
    node->unary.expr = expr;
    return node;
}

static Node *new_node_num(int64_t val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->num.ival = val;
    if (tok) {
        node->ty = tok->val_ty;
    }
    return node;
}

Node *new_node_cast(Node *expr, Type *ty, Token *tok) {
    add_type(expr);

    Node *node = new_node(ND_CAST, tok);
    node->unary.expr = expr;
    node->ty = ty;
    return node;
}

static Node *new_node_var(Obj *var, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
    node->ty = var->kind == GVAR ? expand_pseudo(var->pty) : var->ty;
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
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty)) {
        return new_node_binary(ND_ADD, lhs, rhs, tok);
    }

    if (lhs->ty->base && rhs->ty->base) {
        error_tok(tok, "ポインタ同士の加算はできません");
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
    if (is_numeric(lhs->ty) && is_numeric(rhs->ty)) {
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

    error_tok(tok, "数値からポインタ値を引くことはできません");
}

static void push_to_obj_list(Obj *obj) {
    switch (obj->kind) {
    case LVAR: {
        static Obj *tail;
        if (!locals.next) {
            tail = &locals;
        }
        tail = tail->next = obj;
        return;
    }
    case GVAR: {
        static Obj *tail = &globals;
        tail = tail->next = obj;
        return;
    }
    case FUNC: {
        static Obj *tail = &functions;
        tail = tail->next = obj;
        return;
    }
    }
}

static Obj *new_obj(Token *tok, char *name, Type *ty) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->align = ty->align;
    var->tok = tok;

    // 入力プログラム中に書かれた変数のみを変数スコープに登録する
    if (tok) {
        push_var_into_var_scope(tok, var);
    }
    return var;
}

static Obj *new_lvar(Token *tok, char *name, Type *ty) {
    Obj *var = new_obj(tok, name, ty);
    var->kind = LVAR;
    var->has_definition = true;
    push_to_obj_list(var);
    return var;
}

static Obj *new_gvar(Token *tok, char *name, Type *ty) {
    Obj *var = new_obj(tok, name, ty);
    var->kind = GVAR;
    var->pty = (PseudoType *)ty;
    var->has_definition = true;
    push_to_obj_list(var);
    return var;
}

static Obj *new_func(Token *tok, char *name, Type *ty) {
    Obj *fn = new_obj(tok, name, ty);
    fn->kind = FUNC;
    fn->has_definition = false;
    push_to_obj_list(fn);
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

static Obj *new_str_literal(char *name) {
    static int str_count = 0;
    Obj *var = new_gvar(NULL, format(".Lstr%d", str_count++),
                        array_of(ty_char, (int)strlen(name) + 1));
    var->init_data_str = to_init_data(name);
    return var;
}

// A op= B を `tmp = &A, *tmp = *tmp op B` に変換する
static Node *to_assign(Node *binary) {
    add_type(binary->binop.lhs);
    add_type(binary->binop.rhs);

    Node *var = new_node_var(
        new_lvar(NULL, "", pointer_to(binary->binop.lhs->ty)), NULL);

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

    if (is_any_type_of(ty, 1, TY_ARY)) {
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

    if (is_any_type_of(ty, 2, TY_STRUCT, TY_UNION)) {
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

static PseudoType *declspec(VarAttr *attr);
static TypeIdentPair *declarator(PseudoType *pty);
static Obj *function(PseudoType *pty, Token *tok);
static Node *lvar_initializer(Obj *var);
static void gvar_initializer(Obj *var);
static Node *declaration(void);
static Node *stmt(void);
static Node *compound_stmt(bool is_fn_def);
static Node *expr(void);
static Node *assign(void);
Node *conditional(void);
static Node *logor(void);
static Node *logand(void);
static Node * bitor (void);
static Node *bitxor(void);
static Node *bitand(void);
static Node *equality(void);
static Node *relational(void);
static Node *shift(void);
static Node *add(void);
static Node *mul(void);
static Node *cast(void);
static Node *unary(void);
static Node *postfix(void);
static Node *primary(void);

// program = declaration*
void program(Token *tok) {
    set_token_to_parse(tok);

    push_builtin_obj_into_var_scope();

    while (!at_eof(token)) {
        declaration();
    }
}

static Type *tag_type(Token *tag) {
    Type *ty = calloc(1, sizeof(Type));
    ty->size = -1;
    ty->align = 1;
    ty->name = tag;
    if (tag) {
        push_tag_scope(ty);
    }
    return ty;
}

static Type *tag_type_decl(TypeKind kind) {
    assert(kind == TY_STRUCT || kind == TY_UNION || kind == TY_ENUM);

    Token *tag = consume_ident();
    Type *ty = NULL;
    if (tag) {
        bool in_cur_scope;
        ty = find_tag(tag, &in_cur_scope);
        if (!ty || !in_cur_scope && equal("{", TK_RESERVED, token)) {
            ty = tag_type(tag);
            ty->kind = kind;
            push_tag_scope(ty);
        } else if (in_cur_scope) {
            if (!is_any_type_of(ty, 1, kind)) {
                error_tok(tag, "前回の宣言と型が一致しません");
            }
            if (ty->size > 0 && equal("{", TK_RESERVED, token)) {
                char *kind_str = kind == TY_STRUCT  ? "構造体"
                                 : kind == TY_UNION ? "共用体"
                                                    : "列挙体";
                error_tok(tag, "%sの再定義はできません", kind_str);
            }
        }
    } else {
        ty = tag_type(NULL);
        ty->kind = kind;
    }

    return ty;
}

static bool duplicate_member_exists(Member *list, Token *name) {
    for (Member *m = list; m; m = m->next) {
        // 無名構造体・共用体の場合はそのメンバに対して再帰的に検索
        if (!m->name) {
            assert(m->ty->kind == TY_STRUCT || m->ty->kind == TY_UNION);
            if (duplicate_member_exists(m->ty->members, name)) {
                return true;
            }
            continue;
        }

        if (m->name->len == name->len &&
            !memcmp(m->name->loc, name->loc, name->len)) {
            return true;
        }
    }

    return false;
}

// member = declspec declarator ("," declarator)* ";"
// member-list = member* "}"
static Member *member_list(Type *ty) {
    Member head = {};
    Member *cur = &head;

    int idx = 0;
    while (!consume("}", TK_RESERVED)) {
        VarAttr attr = {};
        PseudoType *pbasety = declspec(&attr);

        int i = 0;
        do {
            if (i++ > 0) {
                expect(",");
            }

            TypeIdentPair *pair = declarator(pbasety);
            if (pair->ident &&
                duplicate_member_exists(head.next, pair->ident)) {
                error_tok(pair->ident, "同名のメンバがすでに存在します");
            }

            Type *mem_ty = expand_pseudo(pair->pty);
            if (mem_ty->size < 0) {
                error_tok(pair->ident, "メンバの型が不完全です");
            }

            Member *mem = new_member(idx++, mem_ty, pair->ident);
            mem->pty = pair->pty;
            if (attr.align) {
                mem->align = attr.align;
            }

            if (ty->align < mem->align) {
                ty->align = mem->align;
            }

            cur = cur->next = mem;
        } while (!equal(";", TK_RESERVED, token));

        expect(";");
    }

    return head.next;
}

// struct-decl = ident? "{" member-list
//             | ident
static Type *struct_decl() {
    Type *ty = tag_type_decl(TY_STRUCT);

    if (!consume("{", TK_RESERVED)) {
        return ty;
    }

    int offset = 0;
    Member *mems = member_list(ty);
    for (Member *mem = mems; mem; mem = mem->next) {
        mem->offset = offset = align_to(offset, mem->align);
        offset += mem->ty->size;
    }

    ty->members = mems;
    ty->size = align_to(offset, ty->align);

    return ty;
}

// union-decl = ident? "{" (declspec declarator ";")* "}"
//             | ident
static Type *union_decl() {
    Type *ty = tag_type_decl(TY_UNION);

    if (!consume("{", TK_RESERVED)) {
        return ty;
    }

    int offset = 0;
    Member *mems = member_list(ty);
    for (Member *mem = mems; mem; mem = mem->next) {
        if (offset < mem->ty->size) {
            offset = mem->ty->size;
        }
    }

    ty->members = mems;
    ty->size = align_to(offset, ty->align);

    return ty;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident
// enum-list = ident ("=" num)? ("," ident ("=" num)?)* ","?
static Type *enum_specifier() {
    Type *ty = tag_type_decl(TY_ENUM);

    if (!consume("{", TK_RESERVED)) {
        return ty;
    }

    int val = 0;
    for (int i = 0; !consume_list_end(); i++) {
        if (i > 0) {
            expect(",");
        }

        Token *tok = expect_ident();
        if (look_in_cur_var_scope(tok)) {
            error_tok(tok, "そのような識別子はすでに存在します");
        }

        if (consume("=", TK_RESERVED)) {
            val = (int)calc_const_expr(assign(), &(char *){NULL} /* dummy */);
        }

        EnumConst *ec = calloc(1, sizeof(EnumConst));
        ec->name = tok;
        ec->val = val++;
        ec->ty = ty;
        push_enum_const_into_var_scope(tok, ec);
    }

    ty->size = 4;
    ty->align = 4;

    return ty;
}

typedef enum {
    STORAGE_CLASS_SPEC = 1,
    TYPE_QUALIFIER,
    TYPE_SPEC,
    ALIGNMENT_SPEC,
} SpecifierKind;

static bool is_storage_class_spec() {
    static char *kw[] = {"typedef", "static", "extern"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(kw[i], TK_KEYWORD, token)) {
            return true;
        }
    }
    return false;
}

static bool is_type_qualifier() {
    static char *kw[] = {"const", "volatile", "restrict"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(kw[i], TK_KEYWORD, token)) {
            return true;
        }
    }
    return false;
}

static bool is_type_spec() {
    static char *kw[] = {"unsigned", "signed",
                         "void",     "int",
                         "char",     "short",
                         "long",     "_Bool",
                         "float",    "double",
                         "struct",   "union",
                         "enum",     "__builtin_va_list"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (equal(kw[i], TK_KEYWORD, token)) {
            return true;
        }
    }
    return false;
}

static bool is_typename() {
    return find_typedef(token) || is_storage_class_spec() ||
           is_type_qualifier() || is_type_spec() ||
           equal("_Alignas", TK_KEYWORD, token);
}

static SpecifierKind get_declspec_kind() {
    if (is_storage_class_spec()) {
        return STORAGE_CLASS_SPEC;
    }

    if (is_type_qualifier()) {
        return TYPE_QUALIFIER;
    }

    if (is_type_spec() || find_typedef(token)) {
        return TYPE_SPEC;
    }

    if (equal("_Alignas", TK_KEYWORD, token)) {
        return ALIGNMENT_SPEC;
    }

    return 0;
}

static bool is_pow2(int n) { return n && !(n & (n - 1)); }

// declspec = ("typedef" | "static" | "extern"
//             | "_Alignas" "(" (type-name | expr) ")"
//             | "unsigned" | "signed"
//             | "void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "float" | "double"
//             | "struct" struct-decl | "union" union-decl
//             | "enum" enum-specifier | typedef-name
//             | "__builtin_va_list"
//             | "const" | "volatile")*
static PseudoType *declspec(VarAttr *attr) {
    enum {
        VOID = 1 << 0,
        BOOL = 1 << 2,
        CHAR = 1 << 4,
        SHORT = 1 << 6,
        INT = 1 << 8,
        LONG = 1 << 10,
        FLOAT = 1 << 12,
        DOUBLE = 1 << 14,
        UNSIGNED = 1 << 16,
        SIGNED = 1 << 18,
        OTHER = 1 << 20,
    };

    Token *tok;
    Type *ty;
    int ty_spec_count = 0;
    bool is_const = false;
    bool is_volatile = false;

    SpecifierKind kind;
    while ((kind = get_declspec_kind())) {
        if (kind == TYPE_QUALIFIER) {
            if ((tok = consume("const", TK_KEYWORD))) {
                if (is_const) {
                    error_tok(tok, "constを複数指定することはできません");
                }
                is_const = true;
                continue;
            }
            if ((tok = consume("volatile", TK_KEYWORD))) {
                if (is_volatile) {
                    error_tok(tok, "volatileを複数指定することはできません");
                }
                is_volatile = true;
                continue;
            }
            if ((tok = consume("restrict", TK_KEYWORD))) {
                error_tok(tok, "ここでrestrictを使うことはできません");
            }
        }

        if (kind == STORAGE_CLASS_SPEC) {
            if (!attr) {
                error_tok(token,
                          "ここで記憶クラス指定子を使うことはできません");
            }

            if ((tok = consume("typedef", TK_KEYWORD))) {
                attr->storage += TYPEDEF;

            } else if ((tok = consume("static", TK_KEYWORD))) {
                attr->storage += STATIC;

            } else if ((tok = consume("extern", TK_KEYWORD))) {
                attr->storage += EXTERN;
            }

            switch (attr->storage) {
            case TYPEDEF:
            case STATIC:
            case EXTERN:
                continue;
            default:
                error_tok(tok,
                          "記憶クラス指定子を複数指定することはできません");
            }
        }

        if (kind == ALIGNMENT_SPEC) {
            if (!attr) {
                error_tok(token, "ここで_Alignasを使うことはできません");
            }

            tok = consume("_Alignas", TK_KEYWORD);
            expect("(");

            attr->align = is_typename() ? expand_pseudo(declspec(NULL))->align
                                        : (int)calc_const_expr(assign(), NULL);
            if (!is_pow2(attr->align)) {
                error_tok(tok, "アライメントが2のべき乗ではありません");
            }

            expect(")");
            continue;
        }

        Type *ty2;
        if ((tok = consume("void", TK_KEYWORD))) {
            ty_spec_count += VOID;

        } else if ((tok = consume("_Bool", TK_KEYWORD))) {
            ty_spec_count += BOOL;

        } else if ((tok = consume("char", TK_KEYWORD))) {
            ty_spec_count += CHAR;

        } else if ((tok = consume("short", TK_KEYWORD))) {
            ty_spec_count += SHORT;

        } else if ((tok = consume("int", TK_KEYWORD))) {
            ty_spec_count += INT;

        } else if ((tok = consume("long", TK_KEYWORD))) {
            ty_spec_count += LONG;

        } else if ((tok = consume("float", TK_KEYWORD))) {
            ty_spec_count += FLOAT;

        } else if ((tok = consume("double", TK_KEYWORD))) {
            ty_spec_count += DOUBLE;

        } else if ((tok = consume("unsigned", TK_KEYWORD))) {
            ty_spec_count += UNSIGNED;

        } else if ((tok = consume("signed", TK_KEYWORD))) {
            ty_spec_count += SIGNED;

        } else if ((tok = consume("struct", TK_KEYWORD))) {
            ty_spec_count += OTHER;
            ty = struct_decl();

        } else if ((tok = consume("union", TK_KEYWORD))) {
            ty_spec_count += OTHER;
            ty = union_decl();

        } else if ((tok = consume("enum", TK_KEYWORD))) {
            ty_spec_count += OTHER;
            ty = enum_specifier();
        } else if ((tok = consume("__builtin_va_list", TK_KEYWORD))) {
            ty_spec_count += OTHER;
            ty = ty_va_list;

        } else if ((ty2 = find_typedef(token))) {
            if (ty_spec_count) {
                break;
            }
            ty_spec_count += OTHER;
            ty = ty2;
            tok = token;
            token = token->next;
        }

        switch (ty_spec_count) {
        case VOID:
            ty = ty_void;
            break;
        case BOOL:
            ty = ty_bool;
            break;
        case CHAR:
        case SIGNED + CHAR:
            ty = ty_char;
            break;
        case UNSIGNED + CHAR:
            ty = ty_uchar;
            break;
        case SHORT:
        case SHORT + INT:
        case SIGNED + SHORT:
        case SIGNED + SHORT + INT:
            ty = ty_short;
            break;
        case UNSIGNED + SHORT:
        case UNSIGNED + SHORT + INT:
            ty = ty_ushort;
            break;
        case INT:
        case SIGNED:
        case SIGNED + INT:
            ty = ty_int;
            break;
        case UNSIGNED:
        case UNSIGNED + INT:
            ty = ty_uint;
            break;
        case LONG:
        case LONG + INT:
        case SIGNED + LONG:
        case SIGNED + LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
        case SIGNED + LONG + LONG:
        case SIGNED + LONG + LONG + INT:
            ty = ty_long;
            break;
        case UNSIGNED + LONG:
        case UNSIGNED + LONG + INT:
        case UNSIGNED + LONG + LONG:
        case UNSIGNED + LONG + LONG + INT:
            ty = ty_ulong;
            break;
        case FLOAT:
            ty = ty_float;
            break;
        case DOUBLE:
        case LONG + DOUBLE:
            ty = ty_double;
            break;
        case OTHER:
            break;
        default:
            error_tok(tok, "型の指定が正しくありません");
        }
    }

    if (is_const) {
        ty = const_of(ty);
    }

    return (PseudoType *)ty;
}

// ary-suffix = "[" assign? "]" ary-suffix?
static PseudoType *ary_suffix(PseudoType *pty) {
    Token *tok;
    if ((tok = consume("[", TK_RESERVED))) {
        if (expand_pseudo(pty)->size < 0) {
            error_tok(tok, "配列の要素の型が不完全です");
        }

        int size =
            !equal("]", TK_RESERVED, token)
                ? (int)calc_const_expr(assign(), &(char *){NULL} /* dummy */)
                : -1;
        expect("]");
        pty = ary_suffix(pty);
        return (PseudoType *)array_of((Type *)pty, size);
    }

    return pty;
}

// func-suffix = "(" func-params? | "void" ")"
// func-params = declspec declarator ("," declspec declarator)* ("," "...")?
static PseudoType *func_suffix(PseudoType *pty) {
    expect("(");

    List params = list_new();
    ListIter begin = list_begin(params);
    bool is_variadic = false;

    while (!equal(")", TK_RESERVED, token)) {
        if (equal("void", TK_KEYWORD, token) &&
            equal(")", TK_RESERVED, token->next)) {
            token = token->next;
            break;
        }

        if (list_size(params)) {
            expect(",");
        }

        if (consume("...", TK_RESERVED)) {
            is_variadic = true;
            break;
        }

        TypeIdentPair *param = declarator(declspec(NULL));
        Type *param_ty = expand_pseudo(param->pty);

        if (is_any_type_of(param_ty, 1, TY_FUNC)) {
            param->pty = (PseudoType *)pointer_to((Type *)param->pty);
        } else if (is_any_type_of(param_ty, 1, TY_ARY)) {
            param->pty = (PseudoType *)pointer_to(((Type *)(param->pty))->base);
        }

        list_push_back(params, param);
    }

    expect(")");

    pty = (PseudoType *)func_type(pty, params);
    ((Type *)(pty))->is_variadic = is_variadic;
    return pty;
}

// type-suffix = ary-suffix | func-suffix | ε
static PseudoType *type_suffix(PseudoType *pty) {
    if (equal("[", TK_RESERVED, token)) {
        return ary_suffix(pty);
    }

    if (equal("(", TK_RESERVED, token)) {
        return func_suffix(pty);
    }

    return pty;
}

// pointers = ("*" ("const" | "volatile" | "restrict")*)*
static PseudoType *pointers(PseudoType *pty) {
    Type *ty = (Type *)pty;

    while (consume("*", TK_RESERVED)) {
        ty = pointer_to(ty);
        Token *tok;
        bool is_volatile = false;
        bool is_restrict = false;

        while (is_type_qualifier()) {
            if ((tok = consume("const", TK_KEYWORD))) {
                if (ty->is_const) {
                    error_tok(tok, "constを複数指定することはできません");
                }
                ty->is_const = true;
                continue;
            }
            if ((tok = consume("volatile", TK_KEYWORD))) {
                if (is_volatile) {
                    error_tok(tok, "volatileを複数指定することはできません");
                }
                is_volatile = true;
                continue;
            }
            if ((tok = consume("restrict", TK_KEYWORD))) {
                if (is_restrict) {
                    error_tok(tok, "restrictを複数指定することはできません");
                }
                is_restrict = true;
                continue;
            }
        }
    }

    return (PseudoType *)ty;
}

// declarator = pointers (ident | "("declarator ")") type-suffix
static TypeIdentPair *declarator(PseudoType *pty) {
    pty = pointers(pty);

    if (consume("(", TK_RESERVED)) {
        Token *start = token;

        PseudoType dummy = {};
        declarator(&dummy);
        expect(")");
        pty = type_suffix(pty);

        token = start;
        TypeIdentPair *pair = declarator(pty);
        expect(")");
        type_suffix(&dummy);
        return pair;
    }

    Token *tok = consume_ident();
    pty = type_suffix(pty);

    TypeIdentPair *pair = calloc(1, sizeof(TypeIdentPair));
    pair->ident = tok;
    pair->pty = pty;
    return pair;
}

// abstract-declarator = pointers ("(" abstract-declarator ")")? type-suffix
static PseudoType *abstract_declarator(PseudoType *pty) {
    pty = pointers(pty);

    if (consume("(", TK_RESERVED)) {
        Token *start = token;

        PseudoType dummy = {};
        abstract_declarator(&dummy);
        expect(")");
        pty = type_suffix(pty);

        token = start;
        pty = abstract_declarator(pty);
        expect(")");
        type_suffix(&dummy);
        return pty;
    }

    return type_suffix(pty);
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

// function-decl = ("{" compound-stmt)?
static Obj *function(PseudoType *pty, Token *tok) {
    Obj *fn = find_func(tok);

    Type *ty = expand_pseudo(pty);
    ty->ret->ty = expand_pseudo(ty->ret->pty);
    for (ListIter it = list_begin(ty->params); it; it = list_next(it)) {
        TypeIdentPair *param = *it;
        param->ty = expand_pseudo(param->pty);
    }

    if (!fn) {
        fn = new_func(tok, get_ident(tok), ty);
    }

    if (!equal("{", TK_RESERVED, token)) {
        return fn;
    }

    fn->ty = ty;

    if (fn->ty->ret->ty->size < 0) {
        error_tok(fn->tok, "戻り値の型が不完全です");
    }
    for (ListIter it = list_begin(fn->ty->params); it; it = list_next(it)) {
        TypeIdentPair *param = *it;
        if (param->ty->size < 0) {
            error_tok(param->ident, "パラメータの型が不完全です");
        }
    }

    cur_fn = fn;
    locals.next = NULL;
    enter_scope();

    VarScope *sc = calloc(1, sizeof(VarScope));
    sc->name = "__func__";
    sc->var = new_str_literal(fn->name);
    sc->next = scope->vars;
    scope->vars = sc;

    for (ListIter it = list_begin(fn->ty->params); it; it = list_next(it)) {
        TypeIdentPair *param = *it;
        new_lvar(param->ident, get_ident(param->ident), param->ty);
        fn->nparams++;
    }

    if (fn->ty->is_variadic) {
        Type *ty = array_of(ty_char, 176);
        ty->align = 16;
        fn->reg_save_area = new_lvar(NULL, "", ty);
    }

    fn->lbrace = expect("{");
    fn->body = compound_stmt(true);
    fn->locals = locals.next;
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
    while (!consume_list_end()) {
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
    if (is_any_type_of(init->ty, 1, TY_ARY)) {
        for (int i = 0; i < init->ty->ary_len; i++) {
            initialize_with_zero(init->children[i]);
        }
        return;
    }

    if (is_any_type_of(init->ty, 2, TY_STRUCT, TY_UNION)) {
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            initialize_with_zero(init->children[mem->idx]);
        }
        return;
    }

    init->expr = new_node_num(0, NULL);
}

// ary-initializer = "{" initializer-list? "}"
//                 | ε
// initializer-list = initializer ("," initializer)* ","?
static void ary_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        return;
    }

    if (init->is_flexible) {
        *init = *new_initializer(
            array_of(init->ty->base, count_ary_init_elems(init->ty)), false);
    }

    int i = 0;
    while (i < init->ty->ary_len && !is_list_end()) {
        if (i > 0) {
            expect(",");
        }

        initializer2(init->children[i++]);
    }

    consume_list_end();
}

// struct-initializer = "{" initializer-list? "}"
//                    | assign
// initializer-list = initializer ("," initializer)* ","?
static void struct_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        Node *expr = assign();
        add_type(expr);
        if (is_any_type_of(expr->ty, 1, TY_STRUCT)) {
            init->expr = expr;
            return;
        }
    }

    Member *mem = init->ty->members;
    while (mem && !is_list_end()) {
        if (mem != init->ty->members) {
            expect(",");
        }

        initializer2(init->children[mem->idx]);
        mem = mem->next;
    }

    consume_list_end();
}

// union-initializer = "{" initializer ","? "}"
//                   | assign
static void union_initializer(Initializer *init) {
    if (!consume("{", TK_RESERVED)) {
        Node *expr = assign();
        add_type(expr);
        if (is_any_type_of(expr->ty, 1, TY_UNION)) {
            init->expr = expr;
            return;
        }
    }

    if (!is_list_end()) {
        initializer2(init->children[0]);
        consume(",", TK_RESERVED);
    }

    consume_list_end();
}

// str-initializer = str
static void str_initializer(Initializer *init) {
    if (init->is_flexible) {
        *init = *new_initializer(
            array_of(init->ty->base, (int)strlen(token->str) + 1), false);
        initialize_with_zero(init);
    }

    int len = MIN(init->ty->ary_len, (int)strlen(token->str) + 1);
    for (int i = 0; i < len; i++) {
        init->children[i]->expr = new_node_num(token->str[i], token);
    }

    token = token->next;
}

// initializer = str-initializer | ary-initializer
//             | struct-initializer | union-initializer | assign
static void initializer2(Initializer *init) {
    if (token->kind == TK_STR) {
        if (is_any_type_of(init->ty, 1, TY_ARY)) {
            str_initializer(init);
            return;
        } else if (is_any_type_of(init->ty, 1, TY_PTR)) {
            Obj *var = new_str_literal(token->str);
            init->expr =
                new_node_unary(ND_ADDR, new_node_var(var, token), token);
            add_type(init->expr);
            token = token->next;
            return;
        } else {
            error_tok(token, "初期化子の型が不正です");
        }
    }

    if (is_any_type_of(init->ty, 1, TY_ARY)) {
        ary_initializer(init);
        return;
    }

    if (is_any_type_of(init->ty, 1, TY_STRUCT)) {
        struct_initializer(init);
        return;
    }

    if (is_any_type_of(init->ty, 1, TY_UNION)) {
        union_initializer(init);
        return;
    }

    if (consume("{", TK_RESERVED)) {
        init->expr = assign();
        add_type(init->expr);
        expect("}");
        return;
    }

    init->expr = assign();
    add_type(init->expr);
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
    if (is_any_type_of(init->ty, 1, TY_ARY)) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (int i = 0; i < init->ty->ary_len; i++) {
            InitDesg desg2 = {desg, i};
            node = new_node_binary(ND_COMMA, node,
                                   create_lvar_init(init->children[i], &desg2),
                                   NULL);
        }

        return node;
    }

    if (is_any_type_of(init->ty, 1, TY_STRUCT) && !init->expr) {
        Node *node = new_node(ND_NULL_EXPR, NULL);
        for (Member *mem = init->ty->members; mem; mem = mem->next) {
            InitDesg desg2 = {desg, 0, mem};
            node = new_node_binary(
                ND_COMMA, node,
                create_lvar_init(init->children[mem->idx], &desg2), NULL);
        }

        return node;
    }

    if (is_any_type_of(init->ty, 1, TY_UNION) && !init->expr) {
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
    InitDesg desg = {NULL, 0, NULL, var};
    return create_lvar_init(init, &desg);
}

static void gvar_initializer(Obj *var) {
    Initializer *init = initializer(var->ty, &var->ty);

    // 変数Aの初期化子が変数Bの場合は変数Bの初期化子を変数Aに設定する
    if (init->expr) {
        if (init->expr->kind == ND_VAR) {
            init = init->expr->var.var->init;
        }
    }

    var->init = init;
}

// declaration = declspec declarator ("=" initializer)?
//                   ("," declarator ("=" initializer)?)* ";"
//             | declspec declarator function-definition
//             | declspec ";"
static Node *declaration() {
    Node head = {};
    Node *cur = &head;

    VarAttr attr = {};
    PseudoType *basety = declspec(&attr);

    for (int i = 0; !equal(";", TK_RESERVED, token); i++) {
        if (i > 0) {
            expect(",");
        }

        TypeIdentPair *pair = declarator(basety);
        Type *ty = expand_pseudo(pair->pty);

        if (!(attr.storage & TYPEDEF) && ty->kind == TY_VOID) {
            error_tok(pair->ident, "void型の変数を宣言することはできません");
        }

        // typedef
        if (attr.storage & TYPEDEF) {
            Type *td = typedef_of((Type *)pair->pty, pair->ident);
            push_typedef_into_var_scope(td->name, td);
            continue;
        }

        // 関数
        if (is_any_type_of(ty, 1, TY_FUNC)) {
            Obj *fn = function(pair->pty, pair->ident);
            fn->is_static = attr.storage & STATIC;
            return NULL;
        }

        // 変数
        Obj *var;
        if (scope->parent) {
            if (attr.storage & STATIC) {
                static int static_count = 0;
                char *name = get_ident(pair->ident);
                char *unique_name =
                    format("%s.%s.%d", cur_fn->name, name, static_count++);
                var = new_gvar(pair->ident, unique_name, ty);
                if (consume("=", TK_RESERVED)) {
                    gvar_initializer(var);
                }

                var->pty =
                    var->ty->kind == TY_ARY ? (PseudoType *)var->ty : pair->pty;
            } else {
                var = new_lvar(pair->ident, get_ident(pair->ident), ty);

                if (consume("=", TK_RESERVED)) {
                    Node *node = new_node(ND_INIT, NULL);
                    node->init.assigns = lvar_initializer(var);
                    cur = cur->next = node;
                }

                if (var->ty->ary_len < 0) {
                    error_tok(pair->ident, "配列の要素数が指定されていません");
                }
            }
            if (var->ty->size < 0) {
                error_tok(var->tok, "不完全な型の変数宣言です");
            }
        } else {
            var = new_gvar(pair->ident, get_ident(pair->ident), ty);
            var->is_static = attr.storage & STATIC;
            var->has_definition = !(attr.storage & EXTERN);
            if (consume("=", TK_RESERVED)) {
                gvar_initializer(var);
            }

            var->pty =
                var->ty->kind == TY_ARY ? (PseudoType *)var->ty : pair->pty;
        }

        if (attr.align) {
            var->align = attr.align;
        }
    }

    expect(";");

    Node *node = new_node(ND_BLOCK, NULL);
    node->block.body = head.next;
    return node;
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

// case-stmt = "case" assign ":" stmt
static Node *case_stmt() {
    Token *tok = consume("case", TK_KEYWORD);
    int64_t val = calc_const_expr(assign(), &(char *){NULL} /* dummy */);

    if (!cur_switch) {
        error_tok(tok, "ここでcase文を使用することはできません");
    }

    for (Node *n = cur_switch->switch_.cases; n; n = n->case_.next) {
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
        node->for_.init = is_typename() ? declaration() : expr_stmt();
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

// do-stmt = "do" stmt "while" "(" expr ")" ";"
static Node *do_stmt() {
    Node *node = new_node(ND_DO, consume("do", TK_KEYWORD));

    int brk_label_id = cur_brk_label_id;
    int cont_label_id = cur_cont_label_id;
    node->do_.brk_label_id = cur_brk_label_id = count();
    node->do_.cont_label_id = cur_cont_label_id = count();
    node->do_.then = stmt();
    cur_brk_label_id = brk_label_id;
    cur_cont_label_id = cont_label_id;

    consume("while", TK_KEYWORD);
    expect("(");
    node->do_.cond = expr();
    expect(")");
    expect(";");

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

// return-stmt = "return" expr? ";"
static Node *return_stmt() {
    Token *tok = consume("return", TK_KEYWORD);
    Node *node = new_node(ND_RETURN, tok);
    bool in_void_fn = is_any_type_of(cur_fn->ty->ret->ty, 1, TY_VOID);
    Node *exp;

    if (equal(";", TK_RESERVED, token)) {
        if (!in_void_fn) {
            error_tok(tok, "非void関数では値を返す必要があります");
        }
        exp = new_node(ND_NULL_EXPR, NULL);
    } else {
        if (in_void_fn) {
            error_tok(tok, "void関数から値を返すことはできません");
        }
        exp = expr();
        add_type(exp);
        exp = new_node_cast(exp, cur_fn->ty->ret->ty, NULL);
    }
    expect(";");

    node->return_.expr = exp;
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
        return compound_stmt(false);
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
    if (equal("do", TK_KEYWORD, token)) {
        return do_stmt();
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

// compound-stmt = (stmt | declaration)* "}"
static Node *compound_stmt(bool is_fn_def) {
    if (!is_fn_def) {
        enter_scope();
    }

    Node head = {};
    for (Node *cur = &head; !equal("}", TK_RESERVED, token);) {
        if (is_typename() && !equal(":", TK_RESERVED, token->next)) {
            cur = cur->next = declaration();
        } else {
            cur = cur->next = stmt();
        }
        add_type(cur);
    }

    if (!is_fn_def) {
        leave_scope();
    }

    Node *node = new_node(ND_BLOCK, expect("}"));
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
Node *conditional() {
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
//      | "(" declspec abstract-declarator ")" "{" initializer-list "}"
//      | unary
static Node *cast() {
    Token *tmp = token;
    if (consume("(", TK_RESERVED)) {
        if (is_typename()) {
            PseudoType *pty = abstract_declarator(declspec(NULL));
            Type *ty = expand_pseudo(pty);
            expect(")");

            // 複合リテラル
            if (equal("{", TK_RESERVED, token)) {
                if (scope->parent) {
                    Obj *var = new_lvar(NULL, "", ty);
                    return new_node_binary(ND_COMMA, lvar_initializer(var),
                                           new_node_var(var, NULL), NULL);
                }

                static int count = 0;
                Obj *var =
                    new_gvar(NULL, format(".Lcompoundliteral%d", count++), ty);
                var->is_static = true;
                gvar_initializer(var);
                return new_node_var(var, NULL);
            }

            return new_node_cast(cast(), ty, NULL);
        }
    }

    token = tmp;
    return unary();
}

// unary = "sizeof" unary
//       | "sizeof" "(" (declspec abstract-declarator) | expr ")"
//       | "_Alignof" "(" declspec abstract-declarator ")"
//       | ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static Node *unary() {
    Token *tok;

    if ((tok = consume("sizeof", TK_KEYWORD))) {
        if (consume("(", TK_RESERVED)) {
            int sz;
            if (is_typename()) {
                Type *ty = expand_pseudo(abstract_declarator(declspec(NULL)));
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

    if ((tok = consume("_Alignof", TK_KEYWORD))) {
        expect("(");
        Type *ty = expand_pseudo(abstract_declarator(declspec(NULL)));
        expect(")");
        return new_node_num(ty->align, tok);
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

static Node *get_ary_elem(Node *var, Node *idx) {
    return new_node_unary(ND_DEREF, new_node_add(var, idx, NULL), var->tok);
}

static Member *get_member(Type *ty) {
    for (Member *cur = ty->members; cur; cur = cur->next) {
        if (!cur->name && (is_any_type_of(ty, 2, TY_STRUCT, TY_UNION))) {
            if (get_member(cur->ty)) {
                return cur;
            }
            continue;
        }
        if (cur->name->len == token->len &&
            !memcmp(cur->name->loc, token->loc, token->len)) {
            return cur;
        }
    }

    return NULL;
}

static Member *copy_mem(Member *mem) {
    Member *ret = calloc(1, sizeof(Member));
    *ret = *mem;
    return ret;
}

static Node *struct_ref(Node *lhs, Token *tok) {
    add_type(lhs);
    if (!is_any_type_of(lhs->ty, 2, TY_STRUCT, TY_UNION)) {
        error_tok(lhs->tok, "構造体/共用体ではありません");
    }

    Type *ty = lhs->ty;
    Node *node = lhs;

    for (;;) {
        Member *mem = get_member(ty);
        if (!mem) {
            error_tok(token, "そのようなメンバは存在しません");
        }

        mem = copy_mem(mem);
        mem->ty = expand_pseudo(mem->pty);
        if (ty->is_const) {
            add_const(&mem->ty);
        }
        if (mem->name) {
            return new_node_member(node, mem, tok);
        } else {
            node = new_node_member(node, mem, NULL);
        }

        ty = mem->ty;
    }
}

bool is_builtin(char *name) {
    char *kw[] = {
        "__builtin_va_start",
        "__builtin_va_arg",
        "__builtin_va_end",
        "__builtin_va_copy",
    };

    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
        if (!strcmp(name, kw[i])) {
            return true;
        }
    }
    return false;
}

static Node *builtin_funcall(Node *fn) {
    if (!strcmp(fn->var.var->name, "__builtin_va_start")) {
        Token *tok = expect("(");
        if (!cur_fn->ty->is_variadic) {
            error_tok(tok, "固定長引数関数で使うことはできません");
        }
        Node *node = new_node(ND_FUNCALL, tok);
        node->funcall.fn = fn;
        node->funcall.args = assign();
        expect(",");
        node->funcall.args->next = assign();
        expect(")");
        return node;
    }

    if (!strcmp(fn->var.var->name, "__builtin_va_arg")) {
        Token *tok = expect("(");
        Node *node = new_node(ND_FUNCALL, tok);
        node->funcall.fn = fn;
        node->funcall.args = assign();
        if (node->funcall.args->ty->kind != TY_VA_LIST) {
            error_tok(node->funcall.args->tok, "va_list型ではありません");
        }
        expect(",");
        node->ty = expand_pseudo(abstract_declarator(declspec(NULL)));
        expect(")");
        return node;
    }

    if (!strcmp(fn->var.var->name, "__builtin_va_end")) {
        Token *tok = expect("(");
        Node *node = new_node(ND_FUNCALL, tok);
        node->funcall.fn = fn;
        node->funcall.args = assign();
        expect(")");
        return node;
    }

    if (!strcmp(fn->var.var->name, "__builtin_va_copy")) {
        Token *tok = expect("(");
        Node *node = new_node(ND_FUNCALL, tok);
        node->funcall.fn = fn;
        node->funcall.args = assign();
        if (node->funcall.args->ty->kind != TY_VA_LIST) {
            error_tok(node->funcall.args->tok, "va_list型ではありません");
        }
        expect(",");
        node->funcall.args->next = assign();
        if (node->funcall.args->next->ty->kind != TY_VA_LIST) {
            error_tok(node->funcall.args->next->tok, "va_list型ではありません");
        }
        expect(")");
        return node;
    }

    unreachable();
}

// funcall = "(" func-args? ")"
// func-args = assign ("," assign)*
static Node *funcall(Node *fn) {
    if (fn->kind == ND_VAR && is_builtin(fn->var.var->name)) {
        return builtin_funcall(fn);
    }

    add_type(fn);

    if (!is_any_type_of(fn->ty, 1, TY_FUNC) &&
        !(is_any_type_of(fn->ty, 1, TY_PTR) &&
          is_any_type_of(fn->ty->base, 1, TY_FUNC))) {
        error_tok(fn->tok, "関数ではありません");
    }

    Node head = {};
    Node *cur = &head;
    Type *ty = is_any_type_of(fn->ty, 1, TY_FUNC) ? fn->ty : fn->ty->base;
    ListIter begin = list_begin(ty->params);
    ListIter it = begin;

    Token *tok = expect("(");
    while (!consume(")", TK_RESERVED)) {
        if (it != begin) {
            expect(",");
        }

        Node *arg = assign();
        add_type(arg);
        if (it) {
            TypeIdentPair *pair = *it;
            cur = cur->next =
                new_node_cast(arg, expand_pseudo(pair->pty), arg->tok);
            it = list_next(it);
        } else {
            cur = cur->next = arg;
        }
    }

    Node *node = new_node(ND_FUNCALL, tok);
    node->ty = ty->ret->ty;
    node->funcall.fn = fn;
    node->funcall.args = head.next;
    return node;
}

// postfix = primary postfix-tail*
// postfix-tail = "++" | "--" | (("." | "->") ident) | ("[" expr "]") | funcall
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
                node->ty, NULL);

        } else if (equal("--", TK_RESERVED, token)) {
            // `A--` は `(A -= 1) + 1` と等価
            add_type(node);
            node = new_node_cast(
                new_node_add(to_assign(new_node_sub(node, node_one, NULL)),
                             node_one, consume("--", TK_RESERVED)),
                node->ty, NULL);

        } else if (equal(".", TK_RESERVED, token)) {
            node = struct_ref(node, consume(".", TK_RESERVED));
            token = token->next;

        } else if (equal("->", TK_RESERVED, token)) {
            node = struct_ref(new_node_unary(ND_DEREF, node, NULL),
                              consume("->", TK_RESERVED));
            token = token->next;

        } else if (consume("[", TK_RESERVED)) {
            node = get_ary_elem(node, expr());
            expect("]");

        } else if (equal("(", TK_RESERVED, token)) {
            node = funcall(node);

        } else {
            return node;
        }
    }
}

// primary = "(" (expr | ("{" compound-stmt)) ")"
//         | ident
//         | str
//         | num
static Node *primary() {
    if (consume("(", TK_RESERVED)) {
        Node *node;
        if (equal("{", TK_RESERVED, token)) { // GNU Statement Exprs
            node = new_node(ND_STMT_EXPR, consume("{", TK_RESERVED));
            node->block.body = compound_stmt(false)->block.body;
        } else {
            node = expr();
        }
        expect(")");
        return node;
    }

    // 識別子
    Token *tok = consume_ident();
    if (tok) {
        // 変数または列挙定数
        VarScope *sc = look_in_var_scope(tok);
        if (!sc || !sc->var && !sc->enum_const) {
            error_tok(tok, "定義されていない変数です");
        }

        if (sc->var) {
            return new_node_var(sc->var, tok);
        } else {
            return new_node_num(sc->enum_const->val, tok);
        }
    }

    // 文字列リテラル
    if (token->kind == TK_STR) {
        Obj *var = new_str_literal(token->str);
        Node *node = new_node_var(var, token);
        token = token->next;
        return node;
    }

    // 数値
    tok = expect_number();
    if (is_flonum(tok->val_ty)) {
        Node *node = new_node(ND_NUM, tok);
        node->num.fval = tok->fval;
        node->ty = tok->val_ty;
        return node;
    } else {
        return new_node_num(tok->ival, tok);
    }

    unreachable();
}
