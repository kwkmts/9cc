#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token Token;
typedef struct Var Var;
typedef struct Function Function;
typedef struct Node Node;
typedef struct Type Type;

//
// tokenize.c
//

void error(char *fmt, ...);

void error_at(const char *loc, char *fmt, ...);

// トークンの種類
typedef enum {
    TK_RESERVED,// 記号
    TK_IDENT,   // 識別子
    TK_NUM,     // 整数
    TK_KEYWORD, // 予約語
    TK_EOF,     // 入力の終わり
} TokenKind;

// トークン型
struct Token {
    TokenKind kind;// トークンの種類
    Token *next;   // 次の入力トークン
    int val;       // kindがTK_NUMの場合、その数値
    char *str;     // トークン文字列
    int len;       // トークンの長さ
};

// 入力プログラム
extern char *user_input;

Token *tokenize();

//
// parse.c
//

// 変数の型
struct Var {
    Var *next; // 次の変数かNULL
    char *name;// 変数名
    Type *ty;  // 型
    int len;   // 名前の長さ
    int offset;// RBPからのオフセット(ローカル変数)
    bool is_lvar;
};

extern Var *locals; // ローカル変数の連結リスト
extern Var *globals;// グローバル変数の連結リスト

// 現在着目しているトークン
extern Token *token;

// 関数の型
struct Function {
    Function *next;// 次の関数かNULL
    char *name;    // 関数名
    Node *body;    // {}内
    Var *params;   // パラメータ
    Var *locals;   // ローカル変数
    int stack_size;
};

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,      // +
    ND_SUB,      // -
    ND_MUL,      // *
    ND_DIV,      // /
    ND_ADDR,     // 単項 &
    ND_DEREF,    // 単項 *
    ND_EQ,       // ==
    ND_NE,       // !=
    ND_LT,       // <
    ND_LE,       // <=
    ND_ASSIGN,   // =
    ND_VAR,      // ローカル変数
    ND_NUM,      // 整数
    ND_FUNCALL,  // 関数呼出
    ND_BLOCK,    // { ... }
    ND_RETURN,   // return
    ND_IF,       // if
    ND_LOOP,     // while, for
    ND_NULL_STMT,// 空文
} NodeKind;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;// ノードの種類
    Type *ty;     // データ型
    Node *lhs;    // 左辺
    Node *rhs;    // 右辺

    int val;// kindがND_NUMの場合、その値

    Var *var;// kindがND_VARの場合

    char *funcname;// kindがND_FUNCALLの場合、関数名
    Node *args;    // kindがND_FUNCALLの場合、その引数リスト

    Node *cond; // 条件式(kindがND_IFかND_LOOP)
    Node *then; // then節(kindがND_IFかND_LOOP)
    Node *els;  // else節(kindがND_IF)
    Node *init; // 初期化式(kindがND_LOOP(for文))
    Node *after;// 更新式(kindがND_LOOP(for文))

    Node *body;// kindがND_BLOCKの場合、{ ... }の中身
    Node *next;//{ ... }の中において、次の式を表す
};

// 関数の連結リスト
extern Function prog;

void program();

//
// type.c
//

// データ型の種類
typedef enum {
    TY_CHAR,
    TY_INT,
    TY_PTR,
    TY_ARY,
} TypeKind;

// データ型の型
struct Type {
    TypeKind kind; // データ型の種類
    int size;      // サイズ
    size_t ary_len;// 配列の要素数
    Type *base;    // データ型がポインタや配列の場合使われる
    Token *name;   // 識別子名
};

extern Type *ty_char;
extern Type *ty_int;

bool is_type_of(TypeKind kind, Type *ty);
bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, size_t len);
void add_type(Node *node);

//
// codegen.c
//

void codegen();
