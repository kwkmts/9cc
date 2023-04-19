#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ___DEBUG
// 下の１行をアンコメントしてデバッグフラグを有効化
// #define ___DEBUG
#endif

typedef struct Token Token;
typedef struct Var Var;
typedef struct Function Function;
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
    char *str;      // トークン文字列
    int len;        // トークンの長さ
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

int64_t calc_const_expr(Node *node);

// 変数の型
struct Var {
    Var *next;           // 次の変数
    Var *scope_next;     // スコープ内での次の変数
    char *name;          // 変数名
    Type *ty;            // 型
    int len;             // 名前の長さ
    int offset;          // RBPからのオフセット(ローカル変数)
    Initializer *init;   // 初期値(グローバル変数)
    char *init_data_str; // 文字列リテラル(グローバル変数)
    bool is_lvar;
};

extern Var *locals;         // ローカル変数の連結リスト
extern Var *globals;        // グローバル変数の連結リスト
extern Function *functions; // 関数の連結リスト

// 現在着目しているトークン
extern Token *token;

// 関数の型
struct Function {
    Function *next; // 次の関数
    char *name;     // 関数名
    Type *ty;       // 型
    int len;        // 名前の長さ
    Node *body;     // {}内
    Var *params;    // パラメータ
    Var *locals;    // ローカル変数
    int stack_size;
    bool has_definition;
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
    ND_CASE,      // case
    ND_LOOP,      // while, for
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

    Node *lhs; // 左辺
    Node *rhs; // 右辺

    int64_t val; // kindがND_NUMの場合、その値

    Var *var; // kindがND_VARの場合

    Member *member; // 構造体のメンバ

    char *funcname; // kindがND_FUNCALLの場合、関数名
    Node *args;     // kindがND_FUNCALLの場合、その引数リスト

    Node *cond;       // 条件式(ND_IF, ND_LOOP, ND_SWITCH)
    Node *then;       // then節(ND_IF, ND_LOOP)
    Node *els;        // else節(ND_IF)
    Node *init;       // 初期化式(for文)
    Node *after;      // 更新式(for文)
    int brk_label_id; // break文のジャンプ先ラベルID(ND_LOOP, ND_SWITCH)
    int cont_label_id;  // continue文のジャンプ先ラベルID(ND_LOOP)
    Node *cases;        // case文リスト(ND_SWITCH)
    Node *default_case; // default文(ND_SWITCH)
    int64_t case_val;   // `case n:`の`n`の値
    Node *case_next;    // 次のcase文

    char *label;      // ラベル名
    int label_id;     // ラベルにつけるユニークな番号
    Node *goto_next;  // 次のgoto文
    Node *label_next; // 次のラベル

    Node *body; // kindがND_BLOCKかND_STMT_EXPRの場合、{ ... }の中身
    Node *next; // { ... }の中において、次の文を表す
};

void program();

//
// type.c
//

// データ型の種類
typedef enum {
    TY_VOID,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_PTR,
    TY_ARY,
    TY_STRUCT,
    TY_UNION,
    TY_FUNC,
} TypeKind;

// データ型の型
struct Type {
    TypeKind kind; // データ型の種類
    int size;      // サイズ
    int align;     // アライメント
    Token *ident;  // 識別子名

    Type *scope_next; // スコープ内での次のタグ
    Token *name;      // タグ名
    Member *members;  // 構造体のメンバリスト

    int ary_len; // 配列の要素数
    Type *base;  // データ型がポインタや配列の場合使われる

    Type *ret;    // 関数型の戻り値のデータ型
    Type *params; // 関数型のパラメータのデータ型リスト
    Type *next;   // 次のパラメータのデータ型
};

struct Member {
    Member *next;
    int idx;
    Type *ty;
    Token *name;
    int offset;
};

extern Type *ty_void;
extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;

bool is_type_of(TypeKind kind, Type *ty);
bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, size_t len);
Type *func_type(Type *ret, Type *params);
void add_type(Node *node);

//
// codegen.c
//

int count();
int align_to(int n, int align);
void codegen();
