#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>
#include "compiler.h"

typedef enum TokenKind {
    TOKEN_END = 0,
    TOKEN_SYMBOL,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY,
    TOKEN_SEMICOLON,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_EXCLAMATION,
    TOKEN_LT_EQUAL,
    TOKEN_GT_EQUAL,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_INVALID,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    char *text;
    size_t text_len;
};

typedef struct TokenLiteral TokenLiteral;
struct TokenLiteral {
    String8 text;
    TokenKind kind;
};

typedef struct Lexer Lexer;
struct Lexer {
    String8 input;
    size_t cursor;
    size_t line;
    size_t beginning_of_line;
};

char lexer_peek_next_char(Lexer *lexer);
Token next_token(Lexer *lexer);
void eat_char(Lexer *lexer);
char *token_kind_name(Token token);

#endif //LEXER_H