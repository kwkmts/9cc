#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// parse.c
//

void error(char *fmt, ...);

void error_at(char *loc, char *fmt, ...);

//トークンの種類
typedef enum {
    TK_RESERVED,  //記号
    TK_IDENT,     //識別子
    TK_NUM,       //整数
    TK_RETURN,    // return
    TK_IF,        // if
    TK_ELSE,      // else
    TK_WHILE,     // while
    TK_FOR,       // for
    TK_EOF,       //入力の終わり
} TokenKind;

typedef struct Token Token;

//トークン型
struct Token {
    TokenKind kind;  //トークンの種類
    Token *next;     //次の入力トークン
    int val;         // kindがTK_NUMの場合、その数値
    char *str;       //トークン文字列
    int len;         //トークンの長さ
};

typedef struct LVar LVar;

//ローカル変数の型
struct LVar {
    LVar *next;  //次の変数かNULL
    char *name;  //変数名
    int len;     //名前の長さ
    int offset;  // RBPからのオフセット
};

//ローカル変数の連結リスト
extern LVar *locals;

//入力プログラム
extern char *user_input;

//現在着目しているトークン
extern Token *token;

Token *tokenize();

//抽象構文木のノードの種類
typedef enum {
    ND_ADD,        // +
    ND_SUB,        // -
    ND_MUL,        // *
    ND_DIV,        // /
    ND_EQ,         // ==
    ND_NE,         // !=
    ND_LT,         // <
    ND_LE,         // <=
    ND_ASSIGN,     // =
    ND_LVAR,       //ローカル変数
    ND_NUM,        // 整数
    ND_BLOCK,      // { ... }
    ND_RETURN,     // return
    ND_IF,         // if
    ND_LOOP,       // while, for
    ND_NULL_STMT,  //空文
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;  //ノードの種類
    Node *lhs;      //左辺
    Node *rhs;      //右辺
    int val;        // kindがND_NUMの場合、その値
    int offset;  // kindがND_LVARの場合、ベースポインタからのオフセット
    Node *cond;   //条件式(kindがND_IFかND_LOOP)
    Node *then;   // then節(kindがND_IFかND_LOOP)
    Node *els;    // else節(kindがND_IF)
    Node *init;   //初期化式(kindがND_LOOP(for文))
    Node *after;  //更新式(kindがND_LOOP(for文))
    Node *body;   // kindがND_BLOCKの場合、{ ... }の中身
    Node *next;   //{ ... }の中において、次の式を表す
};

extern Node *code[100];

void program();

//
// codegen.c
//

void gen_stmt(Node *node);
