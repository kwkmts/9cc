#include "9cc.h"

static Token *token; // 現在着目しているトークン

Token *preprocess(Token *tok) {
    token = tok;
    Token head = {};
    Token *cur = &head;

    while (!at_eof(token)) {
        if (!equal("#", token->kind, token)) {
            cur = cur->next = token;
            token = token->next;
            continue;
        }

        token = token->next;
    }

    cur->next = token;
    return head.next;
}
