#include <stdlib.h>

typedef enum {
    TOKEN_END = 0,
    TOKEN_SYMBOL,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY,
    TOKEN_SEMICOLON,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_LT_EQUAL,
    TOKEN_GT_EQUAL,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_INVALID,
} TokenKind;

typedef struct {
    TokenKind kind;
    char *text;
    size_t text_len;
} Token;

typedef struct {
    char *text;
    TokenKind kind;
} TokenLiteral;

typedef struct {
    char *content;
    size_t content_len;
    size_t cursor;
    size_t line;
    size_t beginning_of_line;
} Lexer;

Token next_token(Lexer *lexer);
void eat_char(Lexer *lexer);
char *token_kind_name(Token token);