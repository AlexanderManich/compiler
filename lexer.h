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
    TOKEN_MINUS,
    TOKEN_MULT,
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
    String8 text;
};

typedef struct TokenLiteral TokenLiteral;
struct TokenLiteral {
    String8 text;
    TokenKind kind;
};

typedef struct Lexer Lexer;
struct Lexer {
    String8 input;
    u64 cursor;
    u64 line;
    u64 beginning_of_line;
};

char lexer_peek_next_char(Lexer *lexer);
Token next_token(Lexer *lexer);
Token peek_token(Lexer *lexer);
void eat_char(Lexer *lexer);
String8 token_kind_name(Token token);

#endif //LEXER_H