#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define unreachable() error("internal error at %s:%d", __FILE__, __LINE__)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// 単方向リスト
typedef struct List *List;
typedef void **ListIter;
List list_new(void);
void list_push_front(List list, void *val);
void list_push_back(List list, void *val);
ListIter list_begin(List list);
ListIter list_next(ListIter it);
int list_size(List list);

// ハッシュマップ
typedef struct Hashmap *Hashmap;
Hashmap hashmap_new(void);

typedef struct Token Token;
typedef struct Obj Obj;
typedef struct Node Node;
typedef struct Type Type;
typedef struct ReturnType ReturnType;
typedef struct Member Member;

// データ型の種類
typedef enum TypeKind {
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_FLOAT,
    TY_DOUBLE,
    TY_PTR,
    TY_ARY,
    TY_STRUCT,
    TY_UNION,
    TY_ENUM,
    TY_FUNC,
    TY_VA_LIST,
    TY_PSEUDO_TYPEDEF,
    TY_PSEUDO_CONST,
} TypeKind;

// データ型の型
struct Type {
    TypeKind kind; // データ型の種類
    int size;      // サイズ
    int align;     // アライメント
    bool is_unsigned;

    Token *name;     // タグ名またはtypedef名
    Member *members; // 構造体のメンバリスト

    int ary_len; // 配列の要素数
    Type *base;  // ポインタ,配列,TY_PSEUDO_*において使われる

    ReturnType *ret; // 関数型の戻り値のデータ型
    List params; // 関数型のパラメータのデータ型(TypeIdentPairのリスト)
    bool is_variadic;

    bool is_const; // const修飾子の有無
};

// Typeのエイリアス(TY_PSEUDO_*なType構造体を含むことを示すために使用される)
typedef Type PseudoType[1];
typedef struct TypeIdentPair TypeIdentPair;

//
// tokenize.c
//

typedef struct {
    char *name;
    int number;
    char *content;
} File;

void error(char *fmt, ...);

void error_at(File *file, char *loc, char *fmt, ...);

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
    int64_t ival;   // kindがTK_NUMの場合、整数
    double fval;    // kindがTK_NUMの場合、浮動小数点数
    Type *val_ty;   // kindがTK_NUMの場合、その型
    char *loc;      // 入力プログラム中での位置
    char *str;      // 文字列リテラル
    int len;        // トークンの長さ
    File *file;     // トークンが含まれるファイル
    int line_no;    // 行番号
    int column_no;  // 列番号
    bool at_bol;    // 行頭かどうか
    bool has_space; // トークンの前に空白があるかどうか
};

extern File **input_files;
Token *copy_token(Token *tok);
void convert_keywords(Token *tok);
File *new_file(char *name, char *content, int number);
Token *tokenize(File *file);
Token *tokenize_file(char *path);

//
// preprocess.c
//

extern Hashmap macros;
void define_macro(char *name, char *buf);
Token *preprocess(Token *tok);

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

extern Obj locals;    // ローカル変数の連結リスト
extern Obj globals;   // グローバル変数の連結リスト
extern Obj functions; // 関数の連結リスト

// 変数・関数
struct Obj {
    enum { LVAR, GVAR, FUNC } kind;
    Obj *next;
    char *name;      // 識別子名
    Type *ty;        // expand_pseudo()によって得られる実質的な型
    PseudoType *pty; // TY_PSEUDO_*を含む型(グローバル変数のみ)
    Token *tok;

    int offset; // RBPからのオフセット

    Initializer *init;   // 初期値
    char *init_data_str; // 文字列リテラル

    Node *body;  // {}内
    int nparams; // パラメータの個数
    Obj *locals; // ローカル変数の連結リスト(先頭からnparams個はパラメータ)
    int stack_size;
    Token *lbrace;      // 関数定義の開始位置("{")
    Obj *reg_save_area; // 可変長引数関数で使われる

    bool is_static;      // グローバル変数・関数で使われる
    bool has_definition; // ローカル変数では常にtrue
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
    ND_DO,        // do
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
            int64_t ival;
            double fval;
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
            Node *cond;        // 条件式
            Node *then;        // then節
            int brk_label_id;  // break文のジャンプ先ラベルID
            int cont_label_id; // continue文のジャンプ先ラベルID
        } do_;

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

void set_token_to_parse(Token *tok);
bool equal(char *op, TokenKind kind, Token *tok);
bool at_eof(Token *tok);
int64_t calc_const_expr(Node *node, char **label);
Node *new_node_unary(NodeKind kind, Node *expr, Token *tok);
Node *new_node_cast(Node *expr, Type *ty, Token *tok);
Node *conditional(void);
void program(Token *tok);
bool is_builtin(char *name);

//
// type.c
//

struct Member {
    Member *next;
    int idx;
    Type *ty;
    PseudoType *pty;
    Token *name;
    int offset;
};

struct ReturnType {
    PseudoType *pty;
    Type *ty;
};

struct TypeIdentPair {
    Token *ident;
    PseudoType *pty;
    Type *ty;
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
extern Type *ty_float;
extern Type *ty_double;
extern Type *ty_va_list;

bool is_any_type_of(Type *ty, int n, ...);
bool is_integer(Type *ty);
bool is_flonum(Type *ty);
bool is_numeric(Type *ty);
Type *copy_type(Type *ty);
Type *expand_pseudo(PseudoType *pty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int len);
Type *typedef_of(Type *base, Token *name);
Type *const_of(Type *base);
Type *func_type(PseudoType *ret, List params);
Member *new_member(int idx, Type *ty, Token *name);
void add_const(Type **ty);
void add_type(Node *node);

//
// codegen.c
//

int count(void);
char *format(const char *fmt, ...);
int align_to(int n, int align);
void codegen(void);

//
// main.c
//

// 動的配列
typedef struct Vector *Vector;
Vector vector_new(void);
void vector_push(Vector vec, void *val);
void *vector_get(Vector vec, int idx);
int vector_size(Vector vec);

extern FILE *outfp;
extern Vector include_paths;

char *read_file(char *path);
