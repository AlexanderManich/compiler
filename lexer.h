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
    TOKEN_EQUAL_EQUAL_EQUAL,
    TOKEN_EXCLAMATION,
    TOKEN_LT_EQUAL,
    TOKEN_GT_EQUAL,
    TOKEN_LT,
    TOKEN_GT,

    //Keywords
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,

    TOKEN_INVALID,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    String8 text;

    u64 beginning_line;
    u64 beginning_char;
    u64 end_line;
    u64 end_char; 
};

typedef struct TokenLiteral TokenLiteral;
struct TokenLiteral {
    String8 text;
    TokenKind kind;
};

// This needs to be a power of 2, because we are masking off bits when calculating the index
// into the array, so it acts like a ring buffer.
#define MAX_CONCURRENT_TOKENS 4

typedef struct Lexer Lexer;
struct Lexer {
    String8 input;
    u64 input_cursor;
    u64 current_line;
    u64 current_char;

    Token tokens[MAX_CONCURRENT_TOKENS];
    u64 token_array_cursor; 
    u64 incoming_tokens_count;
};

TokenKind is_keyword(Token token);

inline Token *get_unused_token(Lexer *lexer) {
    assert(lexer->incoming_tokens_count < MAX_CONCURRENT_TOKENS);

    // Mask off bits so we wrap around if cursor + incoming is bigger than size of array
    u64 index = (lexer->token_array_cursor + lexer->incoming_tokens_count) & (MAX_CONCURRENT_TOKENS - 1);
    Token *result = &lexer->tokens[index];
    result->kind           = TOKEN_INVALID; 
    result->beginning_line = lexer->current_line;
    result->beginning_char = lexer->current_char;

    return result;
}

Token next_token(Lexer *lexer);
char peek_next_char(Lexer *lexer);
Token peek_token(Lexer *lexer);

void eat_char(Lexer *lexer);

TokenKind token_kind_from_lit(String8 str);
String8 token_kind_name(Token token);
Token token_from_kind(TokenKind kind);


#endif //LEXER_H