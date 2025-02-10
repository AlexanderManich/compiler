#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"

TokenLiteral literal_tokens[] = {
    {.text = "(", .kind = TOKEN_LEFT_PAREN},
    {.text = ")", .kind = TOKEN_RIGHT_PAREN},
    {.text = "{", .kind = TOKEN_LEFT_CURLY},
    {.text = "}", .kind = TOKEN_RIGHT_CURLY},
    {.text = ";", .kind = TOKEN_SEMICOLON},
    {.text = "<=", .kind = TOKEN_LT_EQUAL},
    {.text = "=", .kind = TOKEN_ASSIGN},
};

#define literal_tokens_count sizeof(literal_tokens) / sizeof(literal_tokens[0])

char *token_kind_name(Token token) {
    switch (token.kind) {
        case TOKEN_END:
            return "end of input";
        case TOKEN_SYMBOL:
            return "symbol";
        case TOKEN_NUMBER:
            return "number";
        case TOKEN_LEFT_PAREN:
            return "left parenthesis";
        case TOKEN_RIGHT_PAREN:
            return "right parenthesis";
        case TOKEN_LEFT_CURLY:
            return "left curly bracket";
        case TOKEN_RIGHT_CURLY:
            return "right curly bracket";
        case TOKEN_SEMICOLON:
            return "semicolon";
        case TOKEN_LT_EQUAL:
            return "less than equal";
        case TOKEN_ASSIGN:
            return "assignment";
        case TOKEN_INVALID:
            return "invalid token";
        default:
            fprintf(stderr, "ERROR: No name defined for token: %.*s\n", (int) token.text_len, token.text);
    }

    return NULL;
}

bool is_symbol_start(char c) {
    return isalpha(c) || c == '_';
}

bool is_symbol(char c) {
    return isalnum(c) || c == '_';
}

bool is_digit(char c) {
    return isdigit(c);
}

void trim_input_left(Lexer *lexer) {
    while (lexer->cursor < lexer->content_len && isspace(lexer->content[lexer->cursor])) {
        eat_char(lexer);
    }
}

bool starts_with(Lexer *lexer, const char *prefix) {
    size_t prefix_len = strlen(prefix);

    if (prefix == 0) {
        return true;
    }
    if (lexer->cursor + prefix_len - 1 >= lexer->content_len) {
        return false;
    }
     
    for (size_t i = 0; i < prefix_len; i++) {
        if (prefix[i] != lexer->content[lexer->cursor + i]) {
            return false;
        }
    }

    return true;
}

void eat_char(Lexer *lexer) {
    assert(lexer->cursor < lexer->content_len);
    char c = lexer->content[lexer->cursor];
    lexer->cursor += 1;
    if (c == '\n') {
        lexer->line += 1;
        lexer->beginning_of_line = lexer->cursor;
    }
}

Token next_token(Lexer *lexer) {
    trim_input_left(lexer);

    Token token = {
        .text = &lexer->content[lexer->cursor]
    };

    if (lexer->cursor >= lexer->content_len) return token;

    for (size_t i = 0; i < literal_tokens_count; i++) {
        if (starts_with(lexer, literal_tokens[i].text)) {
            size_t text_len = strlen(literal_tokens[i].text);
            token.kind = literal_tokens[i].kind;
            token.text_len = text_len;
            lexer->cursor += text_len;
            return token;
        }
    }

    if (is_symbol_start(lexer->content[lexer->cursor])) {
        token.kind = TOKEN_SYMBOL;
        while (lexer->cursor < lexer->content_len && is_symbol(lexer->content[lexer->cursor])) {
            eat_char(lexer);
            token.text_len += 1;
        }
        return token;
    }

    if (is_digit(lexer->content[lexer->cursor])) {
        token.kind = TOKEN_NUMBER;
        while (lexer->cursor < lexer->content_len && is_digit(lexer->content[lexer->cursor])) {
            eat_char(lexer);
            token.text_len += 1;
        }
        return token;
    }

    eat_char(lexer);
    token.kind = TOKEN_INVALID;
    token.text_len = 1;

    return token;
}


