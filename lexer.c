#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"

// Literals that consist of multiple of the same character need to be earlier in the table,
// than the same single charater, otherwise it will lex the charaters as seperate tokens.
TokenLiteral literal_tokens[] = {
    {.text = str8_lit_comp("("),  .kind = TOKEN_LEFT_PAREN},
    {.text = str8_lit_comp(")"),  .kind = TOKEN_RIGHT_PAREN},
    {.text = str8_lit_comp("{"),  .kind = TOKEN_LEFT_CURLY},
    {.text = str8_lit_comp("}"),  .kind = TOKEN_RIGHT_CURLY},
    {.text = str8_lit_comp(";"),  .kind = TOKEN_SEMICOLON},
    {.text = str8_lit_comp("=="), .kind = TOKEN_EQUAL_EQUAL},
    {.text = str8_lit_comp("="),  .kind = TOKEN_EQUAL},
    {.text = str8_lit_comp("!"),  .kind = TOKEN_EXCLAMATION},
    {.text = str8_lit_comp("<="), .kind = TOKEN_LT_EQUAL},
    {.text = str8_lit_comp(">="), .kind = TOKEN_GT_EQUAL},
    {.text = str8_lit_comp("<"),  .kind = TOKEN_LT},
    {.text = str8_lit_comp(">"),  .kind = TOKEN_GT},
    {.text = str8_lit_comp("+"),  .kind = TOKEN_PLUS},
    {.text = str8_lit_comp("-"),  .kind = TOKEN_MINUS},
    {.text = str8_lit_comp("*"),  .kind = TOKEN_MULT},
};

String8 keywords[] = {
    str8_lit_comp("if"),
    str8_lit_comp("else"),
    str8_lit_comp("return"),
};

#define literal_tokens_count sizeof(literal_tokens) / sizeof(literal_tokens[0])
#define keywords_count sizeof(keywords) / sizeof(keywords[0])

String8 token_kind_name(Token token) {
    switch (token.kind) {
        case TOKEN_END:
            return str8_lit("end of input");
        case TOKEN_SYMBOL:
            return str8_lit("symbol");
        case TOKEN_NUMBER:
            return str8_lit("number");
        case TOKEN_STRING:
            return str8_lit("string");
        case TOKEN_PLUS:
            return str8_lit("plus");
        case TOKEN_MINUS:
            return str8_lit("minus");
        case TOKEN_MULT:
            return str8_lit("mult");
        case TOKEN_LEFT_PAREN:
            return str8_lit("left parenthesis");
        case TOKEN_RIGHT_PAREN:
            return str8_lit("right parenthesis");
        case TOKEN_LEFT_CURLY:
            return str8_lit("left curly bracket");
        case TOKEN_RIGHT_CURLY:
            return str8_lit("right curly bracket");
        case TOKEN_SEMICOLON:
            return str8_lit("semicolon");
        case TOKEN_EQUAL:
            return str8_lit("equal");
        case TOKEN_EQUAL_EQUAL:
            return str8_lit("equal equal");
        case TOKEN_EXCLAMATION:
            return str8_lit("exclamation mark");
        case TOKEN_LT_EQUAL:
            return str8_lit("less than equal");
        case TOKEN_GT_EQUAL:
            return str8_lit("greater than equal");
        case TOKEN_LT:
            return str8_lit("less than");
        case TOKEN_GT:
            return str8_lit("greater than");

        // Keywords
        case TOKEN_KEYWORD_RETURN:
            return str8_lit("return");

        case TOKEN_INVALID:
            return str8_lit("invalid token");
        default:
            fprintf(stderr, "ERROR: No name defined for token: %.*s\n", (int) token.text.size, token.text.str);
    }

    return (String8){0};
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

// Temp Hack
Token peek_token(Lexer *lexer) {
    Lexer temp_lexer = *lexer;
    return next_token(&temp_lexer);
}

inline 
Token next_token(Lexer *lexer) {
    if (!lexer->input.size) return (Token){0};

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
    
    // Literals
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

    // Symbols and keywords
    if (is_symbol_start(lexer->input.str[lexer->cursor])) {
        while (lexer->cursor < lexer->input.size && is_symbol(lexer->input.str[lexer->cursor])) {
            eat_char(lexer);
            token.text.size += 1;
        }
        token.kind = TOKEN_SYMBOL;
        
        // Keywords
        for (u32 i = 0; i < keywords_count; ++i) {
            if (str8_compare(token.text, keywords[i])) {
                token.kind = token_kind_from_lit(token.text);
            }
        }

        return token;
    }

    // Numbers 
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

TokenKind token_kind_from_lit(String8 str) {
    if (str8_compare(str, str8_lit("return"))) return TOKEN_KEYWORD_RETURN;
    if (str8_compare(str, str8_lit("if"))) return TOKEN_KEYWORD_IF;
    if (str8_compare(str, str8_lit("else"))) return TOKEN_KEYWORD_ELSE;

    return 0;
}

Token token_from_kind(TokenKind kind) {
    switch (kind) {
        case TOKEN_LEFT_PAREN:
            return token_lit(kind, "(");
        case TOKEN_RIGHT_PAREN:
            return token_lit(kind, ")");
        case TOKEN_LEFT_CURLY:
            return token_lit(kind, "{");
        case TOKEN_RIGHT_CURLY:
            return token_lit(kind, "}");
        case TOKEN_SEMICOLON:
            return token_lit(kind, ";");
        case TOKEN_SYMBOL:
            return token_lit(kind, "Symbol");    
        default:
            fprintf(stderr, "Token from kind (%d) not implemented\n", kind);
            return (Token){0};
    }
}

