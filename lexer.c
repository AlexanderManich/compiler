#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"

// Literals that consist of multiple of the same character need to be earlier in the table,
// than the same single charater, otherwise it will lex the charaters as seperate tokens.
TokenLiteral literal_tokens[] = {
    {.text = "(",  .kind = TOKEN_LEFT_PAREN},
    {.text = ")",  .kind = TOKEN_RIGHT_PAREN},
    {.text = "{",  .kind = TOKEN_LEFT_CURLY},
    {.text = "}",  .kind = TOKEN_RIGHT_CURLY},
    {.text = ";",  .kind = TOKEN_SEMICOLON},
    {.text = "==", .kind = TOKEN_EQUAL_EQUAL},
    {.text = "=",  .kind = TOKEN_EQUAL},
    {.text = "<=", .kind = TOKEN_LT_EQUAL},
    {.text = ">=", .kind = TOKEN_GT_EQUAL},
    {.text = "<",  .kind = TOKEN_LT},
    {.text = ">",  .kind = TOKEN_GT},
    {.text = "+",  .kind = TOKEN_PLUS},
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
        case TOKEN_STRING:
            return "string";
        case TOKEN_PLUS:
            return "plus";
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
        case TOKEN_EQUAL:
            return "equal";
        case TOKEN_EQUAL_EQUAL:
            return "equal equal";
        case TOKEN_LT_EQUAL:
            return "less than equal";
        case TOKEN_GT_EQUAL:
            return "greater than equal";
        case TOKEN_LT:
            return "less than";
        case TOKEN_GT:
            return "greater than";
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

bool is_number_start(char c) {
    if (isdigit(c)) return true;
    return false;
}

void trim_input_left(Lexer *lexer) {
    while (lexer->cursor < lexer->input.size && isspace(lexer->input.str[lexer->cursor])) {
        eat_char(lexer);
    }
}

bool lexer_starts_with(Lexer *lexer, const char *prefix) {
    size_t prefix_len = strlen(prefix);

    if (prefix == 0) {
        return true;
    }
    if (lexer->cursor + prefix_len - 1 >= lexer->input.size) {
        return false;
    }
     
    for (size_t i = 0; i < prefix_len; i++) {
        if (prefix[i] != lexer->input.str[lexer->cursor + i]) {
            return false;
        }
    }

    return true;
}

inline 
void eat_char(Lexer *lexer) {
    assert(lexer->cursor < lexer->input.size);
    char c = lexer->input.str[lexer->cursor];
    lexer->cursor += 1;
    if (c == '\n') {
        lexer->line += 1;
        lexer->beginning_of_line = lexer->cursor;
    }
}

inline 
char lexer_peek_next_char(Lexer *lexer) {
    return lexer->input.str[lexer->cursor + 1];
}

Token next_token(Lexer *lexer) {
    trim_input_left(lexer);
    
    if (lexer->input.str[lexer->cursor] == '/') {
        if (lexer_peek_next_char(lexer) == '/') {
            size_t comment_line = lexer->line;
            while (comment_line == lexer->line) {
                eat_char(lexer);
            }
        }
    }
    
    trim_input_left(lexer);
    
    Token token = {
        .text = &lexer->input.str[lexer->cursor]
    };
    
    if (lexer->cursor >= lexer->input.size) return token;
    
    // Strings
    // TODO: This does not work correctly
    if (lexer->input.str[lexer->cursor] == '"') {
        token.kind = TOKEN_STRING;
        eat_char(lexer); 
        token.text = &lexer->input.str[lexer->cursor]; 
        while (lexer->cursor < lexer->input.size && lexer->input.str[lexer->cursor] != '"') {
            eat_char(lexer);
            token.text_len += 1;
        }
        eat_char(lexer); 

        return token;
    }
    
    for (size_t i = 0; i < literal_tokens_count; i++) {
        // If current character is a prefix of a literal, lex the whole literal
        if (lexer_starts_with(lexer, literal_tokens[i].text)) {
            size_t text_len = strlen(literal_tokens[i].text);
            token.kind = literal_tokens[i].kind;
            token.text_len = text_len;
            lexer->cursor += text_len;
            return token;
        }
    }

    if (is_symbol_start(lexer->input.str[lexer->cursor])) {
        token.kind = TOKEN_SYMBOL;
        while (lexer->cursor < lexer->input.size && is_symbol(lexer->input.str[lexer->cursor])) {
            eat_char(lexer);
            token.text_len += 1;
        }
        return token;
    }

    if (is_number_start(lexer->input.str[lexer->cursor])) {
        token.kind = TOKEN_NUMBER;
        while (lexer->cursor < lexer->input.size && is_number_start(lexer->input.str[lexer->cursor])) {
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


