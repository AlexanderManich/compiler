#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"

// Literals that consist of multiple of the same character need to be earlier in the table,
// than the same single charater, otherwise it will lex the charaters as seperate tokens.
TokenLiteral literal_tokens[] = {
    {.text = S8LitComp("("),  .kind = TOKEN_LEFT_PAREN},
    {.text = S8LitComp(")"),  .kind = TOKEN_RIGHT_PAREN},
    {.text = S8LitComp("{"),  .kind = TOKEN_LEFT_CURLY},
    {.text = S8LitComp("}"),  .kind = TOKEN_RIGHT_CURLY},
    {.text = S8LitComp(";"),  .kind = TOKEN_SEMICOLON},
    {.text = S8LitComp("=="), .kind = TOKEN_EQUAL_EQUAL},
    {.text = S8LitComp("="),  .kind = TOKEN_EQUAL},
    {.text = S8LitComp("!"),  .kind = TOKEN_EXCLAMATION},
    {.text = S8LitComp("<="), .kind = TOKEN_LT_EQUAL},
    {.text = S8LitComp(">="), .kind = TOKEN_GT_EQUAL},
    {.text = S8LitComp("<"),  .kind = TOKEN_LT},
    {.text = S8LitComp(">"),  .kind = TOKEN_GT},
    {.text = S8LitComp("+"),  .kind = TOKEN_PLUS},
};

#define literal_tokens_count sizeof(literal_tokens) / sizeof(literal_tokens[0])

String8 token_kind_name(Token token) {
    switch (token.kind) {
        case TOKEN_END:
            return S8Lit("end of input");
        case TOKEN_SYMBOL:
            return S8Lit("symbol");
        case TOKEN_NUMBER:
            return S8Lit("number");
        case TOKEN_STRING:
            return S8Lit("string");
        case TOKEN_PLUS:
            return S8Lit("plus");
        case TOKEN_LEFT_PAREN:
            return S8Lit("left parenthesis");
        case TOKEN_RIGHT_PAREN:
            return S8Lit("right parenthesis");
        case TOKEN_LEFT_CURLY:
            return S8Lit("left curly bracket");
        case TOKEN_RIGHT_CURLY:
            return S8Lit("right curly bracket");
        case TOKEN_SEMICOLON:
            return S8Lit("semicolon");
        case TOKEN_EQUAL:
            return S8Lit("equal");
        case TOKEN_EQUAL_EQUAL:
            return S8Lit("equal equal");
        case TOKEN_EXCLAMATION:
            return S8Lit("exclamation mark");
        case TOKEN_LT_EQUAL:
            return S8Lit("less than equal");
        case TOKEN_GT_EQUAL:
            return S8Lit("greater than equal");
        case TOKEN_LT:
            return S8Lit("less than");
        case TOKEN_GT:
            return S8Lit("greater than");
        case TOKEN_INVALID:
            return S8Lit("invalid token");
        default:
            fprintf(stderr, "ERROR: No name defined for token: %.*s\n", (int) token.text.size, token.text.str);
    }

    return S8Lit("");
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

bool lexer_starts_with(Lexer *lexer, const String8 prefix) {
    if (prefix.str == 0) {
        return true;
    }
    if (lexer->cursor + prefix.size - 1 >= lexer->input.size) {
        return false;
    }
     
    for (size_t i = 0; i < prefix.size; i++) {
        if (prefix.str[i] != lexer->input.str[lexer->cursor + i]) {
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
        .text.str = &lexer->input.str[lexer->cursor]
    };
    
    if (lexer->cursor >= lexer->input.size) return token;
    
    // Strings
    // TODO: This does not work correctly
    if (lexer->input.str[lexer->cursor] == '"') {
        token.kind = TOKEN_STRING;
        eat_char(lexer); 
        token.text.str = &lexer->input.str[lexer->cursor]; 
        while (lexer->cursor < lexer->input.size && lexer->input.str[lexer->cursor] != '"') {
            eat_char(lexer);
            token.text.size += 1;
        }
        eat_char(lexer); 

        return token;
    }
    
    for (size_t i = 0; i < literal_tokens_count; i++) {
        // If current character is a prefix of a literal, lex the whole literal
        if (lexer_starts_with(lexer, literal_tokens[i].text)) {
            size_t text_len = literal_tokens[i].text.size;
            token.kind = literal_tokens[i].kind;
            token.text.size = text_len;
            lexer->cursor += text_len;
            return token;
        }
    }

    if (is_symbol_start(lexer->input.str[lexer->cursor])) {
        token.kind = TOKEN_SYMBOL;
        while (lexer->cursor < lexer->input.size && is_symbol(lexer->input.str[lexer->cursor])) {
            eat_char(lexer);
            token.text.size += 1;
        }
        return token;
    }

    if (is_number_start(lexer->input.str[lexer->cursor])) {
        token.kind = TOKEN_NUMBER;
        while (lexer->cursor < lexer->input.size && is_number_start(lexer->input.str[lexer->cursor])) {
            eat_char(lexer);
            token.text.size += 1;
        }
        return token;
    }

    eat_char(lexer);
    token.kind = TOKEN_INVALID;
    token.text.size = 1;

    return token;
}


