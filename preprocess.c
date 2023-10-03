#include "9cc.h"

static Token *token; // 現在着目しているトークン

typedef struct CondIncl CondIncl;
struct CondIncl {
    CondIncl *parent;
    Token *tok;
};

static CondIncl *cond_incl;

static Token *consume(char *op, TokenKind kind) {
    if (!equal(op, kind, token)) {
        return false;
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

static bool consume_hash() {
    if (!equal("#", TK_RESERVED, token) || !token->at_bol) {
        return false;
    }
    token = token->next;
    return true;
}

static Token *expect_integer() {
    if (token->kind != TK_NUM) {
        error_tok(token, "数ではありません");
    }
    if (is_flonum(token->val_ty)) {
        error_tok(token, "浮動小数点数は使えません");
    }
    Token *tok = token;
    token = token->next;
    return tok;
}

// tok2をtok1の末尾に連結する
static Token *append(Token *tok1, Token *tok2) {
    if (!tok1 || tok1->kind == TK_EOF) {
        return tok2;
    }

    Token *cur = tok1;
    while (cur->next->kind != TK_EOF) {
        cur = cur->next;
    }
    cur->next = tok2;

    return tok1;
}

static CondIncl *push_cond_incl(Token *tok) {
    CondIncl *ci = calloc(1, sizeof(CondIncl));
    ci->parent = cond_incl;
    ci->tok = tok;
    cond_incl = ci;
    return ci;
}

static Token *skip_until_endif() {
    while (!at_eof(token)) {
        if (equal("#", TK_RESERVED, token) &&
            equal("endif", TK_IDENT, token->next)) {
            return token;
        }

        if (equal("#", TK_RESERVED, token) &&
            equal("if", TK_KEYWORD, token->next)) {
            token = token->next->next;
            token = skip_until_endif();
            if (!at_eof(token)) {
                token = token->next;
            }
            continue;
        }

        token = token->next;
    }
    return token;
}

Token *preprocess(Token *tok) {
    token = tok;
    Token head = {};
    Token *cur = &head;

    while (!at_eof(token)) {
        Token *tok;

        if (!consume_hash()) {
            cur = cur->next = token;
            token = token->next;
            continue;
        }

        if (consume("include", TK_IDENT)) {
            if (token->kind != TK_STR) {
                error_tok(token, "\"ファイル名\" ではありません");
            }

            char *path =
                format("%s/%s", dirname(strdup(token->file->name)), token->str);
            Token *tok2 = tokenize_file(path);
            token = append(tok2, token->next);
            continue;
        }

        if ((tok = consume("if", TK_KEYWORD))) {
            // TODO:定数式を取れるようにする
            int64_t val = expect_integer()->ival;
            push_cond_incl(tok);
            if (!val) {
                token = skip_until_endif();
            }
            continue;
        }

        if ((tok = consume("endif", TK_IDENT))) {
            if (!cond_incl) {
                error_tok(tok, "対応する#ifがありません");
            }

            cond_incl = cond_incl->parent;
            continue;
        }

        if (token->at_bol) {
            continue;
        }

        error_tok(token, "不正なディレクティブです");
    }

    if (cond_incl) {
        error_tok(cond_incl->tok, "対応する#endifがありません");
    }

    cur->next = token;
    return head.next;
}
