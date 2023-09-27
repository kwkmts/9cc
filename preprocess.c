#include "9cc.h"

static Token *token; // 現在着目しているトークン

static bool consume(char *op) {
    if (!equal(op, TK_IDENT, token)) {
        return false;
    }
    token = token->next;
    return true;
}

static bool consume_hash() {
    if (!equal("#", TK_RESERVED, token) || !token->at_bol) {
        return false;
    }
    token = token->next;
    return true;
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

Token *preprocess(Token *tok) {
    token = tok;
    Token head = {};
    Token *cur = &head;

    while (!at_eof(token)) {
        if (!consume_hash()) {
            cur = cur->next = token;
            token = token->next;
            continue;
        }

        if (consume("include")) {
            if (token->kind != TK_STR) {
                error_tok(token, "\"ファイル名\" ではありません");
            }

            char *path =
                format("%s/%s", dirname(strdup(token->file->name)), token->str);
            Token *tok2 = tokenize_file(path);
            token = append(tok2, token->next);
            continue;
        }

        if (token->at_bol) {
            continue;
        }

        error_tok(token, "不正なディレクティブです");
    }

    cur->next = token;
    return head.next;
}
