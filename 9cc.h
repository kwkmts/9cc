#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct Token Token;
typedef struct Var Var;
typedef struct Function Function;
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Type Type;
typedef struct Member Member;

//
// tokenize.c
//

void error(char *fmt, ...);

void error_at(char *loc, char *fmt, ...);

void error_tok(Token *tok, char *fmt, ...);

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数
    TK_STR,      // 文字列リテラル
    TK_KEYWORD,  // 予約語
    TK_EOF,      // 入力の終わり
} TokenKind;

// トークン型
struct Token {
    TokenKind kind; // トークンの種類
    Token *next;    // 次の入力トークン
    int64_t val;    // kindがTK_NUMの場合、その数値
    char *loc;      // 入力プログラム中での位置
    char *str;      // 文字列リテラル
    int len;        // トークンの長さ
    int line_no;    // 行番号
    int column_no;  // 列番号
};

extern char *filepath;   // ソースファイルのパス
extern char *user_input; // 入力プログラム

Token *tokenize();

//
// parse.c
//

// 初期化子の型
typedef struct Initializer Initializer;
struct Initializer {
    Type *ty;
    Node *expr;
    Initializer **children;
    bool is_flexible;
};

int64_t calc_const_expr(Node *node, char **label);
Node *new_node_cast(Node *expr, Type *ty);

extern Obj locals;    // ローカル変数の連結リスト
extern Obj globals;   // グローバル変数の連結リスト
extern Obj functions; // 関数の連結リスト

// 現在着目しているトークン
extern Token *token;

// 変数・関数
struct Obj {
    enum { LVAR, GVAR, FUNC } kind;
    Obj *next;
    char *name; // 識別子名
    Type *ty;

    int offset;          // RBPからのオフセット

    Initializer *init;   // 初期値
    char *init_data_str; // 文字列リテラル

    Node *body;          // {}内
    int nparams;         // パラメータの個数
    Obj *locals; // ローカル変数の連結リスト(先頭からnparams個はパラメータ)
    int stack_size;
    Token *lbrace;       // 関数定義の開始位置("{")

    bool is_static;      // グローバル変数・関数で使われる
    bool has_definition; // グローバル変数・関数で使われる
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_MOD,       // %
    ND_ADDR,      // 単項 &
    ND_DEREF,     // 単項 *
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_ASSIGN,    // =
    ND_NOT,       // !
    ND_LOGAND,    // &&
    ND_LOGOR,     // ||
    ND_BITNOT,    // ~
    ND_BITAND,    // &
    ND_BITOR,     // |
    ND_BITXOR,    // ^
    ND_SHL,       // <<
    ND_SHR,       // >>
    ND_COMMA,     // ,
    ND_MEMBER,    // .
    ND_CAST,      // キャスト
    ND_COND,      // 条件演算子 ?:
    ND_VAR,       // ローカル変数
    ND_NUM,       // 整数
    ND_FUNCALL,   // 関数呼出
    ND_BLOCK,     // { ... }
    ND_GOTO,      // goto, break, continue
    ND_LABEL,     // ラベル
    ND_RETURN,    // return
    ND_IF,        // if
    ND_SWITCH,    // switch
    ND_CASE,      // case, default
    ND_WHILE,     // while
    ND_FOR,       // for
    ND_INIT,      // 初期化
    ND_EXPR_STMT, // 式文
    ND_STMT_EXPR, // GNU Statement Exprs
    ND_NULL_EXPR, // 何もしない式
    ND_NULL_STMT, // 空文
} NodeKind;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの種類
    Type *ty;      // データ型
    Token *tok;    // 対応するトークン
    Node *next;    // { ... }の中において、次の文を表す

    union {
        struct {
            Node *lhs; // 左辺
            Node *rhs; // 右辺
        } binop;

        struct {
            Node *expr; // 式
        } unary;

        struct {
            Node *cond; // 条件式
            Node *then; // then節
            Node *els;  // else節
        } condop;

        struct {
            Node *lhs;   // 左辺
            Member *mem; // メンバ
        } member;

        struct {
            int64_t val;
        } num;

        struct {
            Obj *var;
        } var;

        struct {
            Node *fn;
            Node *args; // 引数リスト
        } funcall;

        // ND_BLOCK, ND_STMT_EXPR
        struct {
            Node *body;
        } block;

        struct {
            Node *cond; // 条件式
            Node *then; // then節
            Node *els;  // else節
        } if_;

        struct {
            Node *cond;       // 条件式
            Node *then;       // then節
            Node *cases;      // case文リスト
            Node *default_;   // default文
            Node *case_next;  // 次のcase文
            int brk_label_id; // break文のジャンプ先ラベルID
        } switch_;

        struct {
            int64_t val; // `case n:`の`n`の値
            int id;      // case文のID(ラベルと共通)
            Node *stmt;  // case文に続く文
            Node *next;  // 次のcase文
        } case_;

        struct {
            Node *cond;        // 条件式
            Node *then;        // then節
            int brk_label_id;  // break文のジャンプ先ラベルID
            int cont_label_id; // continue文のジャンプ先ラベルID
        } while_;

        struct {
            Node *init;        // 初期化式
            Node *cond;        // 条件式
            Node *after;       // 更新式
            Node *then;        // then節
            int brk_label_id;  // break文のジャンプ先ラベルID
            int cont_label_id; // continue文のジャンプ先ラベルID
        } for_;

        struct {
            char *label; // 対応するラベル名
            int id;      // goto文のジャンプ先ラベルID
            Node *next;  // 次のgoto文
        } goto_;

        struct {
            char *name; // ラベル名
            int id;     // ラベルID
            Node *stmt; // ラベルがついている文
            Node *next; // 次のラベル
        } label;

        struct {
            Node *expr;
        } return_;

        struct {
            Node *expr;
        } exprstmt;

        struct {
            Node *assigns;
        } init;
    };
};

void program();

//
// type.c
//

// データ型の種類
typedef enum {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_PTR,
    TY_ARY,
    TY_STRUCT,
    TY_UNION,
    TY_ENUM,
    TY_FUNC,
} TypeKind;

// データ型の型
struct Type {
    TypeKind kind;   // データ型の種類
    int size;        // サイズ
    int align;       // アライメント
    bool is_unsigned;
    Token *ident;    // 識別子名

    Token *name;     // タグ名またはtypedef名
    Member *members; // 構造体のメンバリスト

    int ary_len;     // 配列の要素数
    Type *base;   // データ型がポインタや配列の場合使われる

    Type *ret;    // 関数型の戻り値のデータ型
    Type *params; // 関数型のパラメータのデータ型リスト
    Type *next;   // 次のパラメータのデータ型
    bool is_variadic;

    bool is_const; // const修飾子の有無
};

struct Member {
    Member *next;
    int idx;
    Type *ty;
    Token *name;
    int offset;
};

extern Type *ty_void;
extern Type *ty_bool;
extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;
extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;

bool is_type_of(TypeKind kind, Type *ty);
bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int len);
Type *enum_type();
Type *func_type(Type *ret, Type *params);
void add_const(Type **ty);
void add_type(Node *node);

//
// codegen.c
//

int count();
char *format(const char *fmt, ...);
int align_to(int n, int align);
void codegen();
