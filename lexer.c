#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "lexer.h"

// Literals that consist of multiple of the same character need to be earlier in the table,
// than the same single charater, otherwise it will lex the charaters as seperate tokens.
// Since we are using designated initializers we need to make sure this is true in the token kind enum.
TokenLiteral token_literals[] = {
    [TOKEN_LEFT_PAREN]  = {.symbol = str8_lit_comp("("),  .kind = TOKEN_LEFT_PAREN,  .precedence = 150},
    [TOKEN_RIGHT_PAREN] = {.symbol = str8_lit_comp(")"),  .kind = TOKEN_RIGHT_PAREN, .precedence = 150},
    [TOKEN_LEFT_CURLY]  = {.symbol = str8_lit_comp("{"),  .kind = TOKEN_LEFT_CURLY,  .precedence = -1},
    [TOKEN_RIGHT_CURLY] = {.symbol = str8_lit_comp("}"),  .kind = TOKEN_RIGHT_CURLY, .precedence = -1},
    [TOKEN_SEMICOLON]   = {.symbol = str8_lit_comp(";"),  .kind = TOKEN_SEMICOLON,   .precedence = -1},
    [TOKEN_COLON_COLON] = {.symbol = str8_lit_comp("::"), .kind = TOKEN_COLON_COLON, .precedence = -1},
    [TOKEN_COLON]       = {.symbol = str8_lit_comp(":"),  .kind = TOKEN_COLON,       .precedence = -1},
    [TOKEN_ARROW]       = {.symbol = str8_lit_comp("->"), .kind = TOKEN_ARROW,       .precedence = -1},
    [TOKEN_COMMA]       = {.symbol = str8_lit_comp(","),  .kind = TOKEN_COMMA,       .precedence = 0},
    [TOKEN_EQUAL_EQUAL] = {.symbol = str8_lit_comp("=="), .kind = TOKEN_EQUAL_EQUAL, .precedence = 90},
    [TOKEN_EQUAL]       = {.symbol = str8_lit_comp("="),  .kind = TOKEN_EQUAL,       .precedence = 10},
    [TOKEN_EXCLAMATION] = {.symbol = str8_lit_comp("!"),  .kind = TOKEN_EXCLAMATION, .precedence = 140},
    [TOKEN_LT_EQUAL]    = {.symbol = str8_lit_comp("<="), .kind = TOKEN_LT_EQUAL,    .precedence = 80},
    [TOKEN_GT_EQUAL]    = {.symbol = str8_lit_comp(">="), .kind = TOKEN_GT_EQUAL,    .precedence = 80},
    [TOKEN_LT]          = {.symbol = str8_lit_comp("<"),  .kind = TOKEN_LT,          .precedence = 80},
    [TOKEN_GT]          = {.symbol = str8_lit_comp(">"),  .kind = TOKEN_GT,          .precedence = 80},
    [TOKEN_PLUS]        = {.symbol = str8_lit_comp("+"),  .kind = TOKEN_PLUS,        .precedence = 120},
    [TOKEN_MINUS]       = {.symbol = str8_lit_comp("-"),  .kind = TOKEN_MINUS,       .precedence = 120},
    [TOKEN_MULT]        = {.symbol = str8_lit_comp("*"),  .kind = TOKEN_MULT,        .precedence = 130},
};

String8 token_kind_name(TokenKind kind) {
    switch (kind) {
        case TOKEN_END:
            return str8_lit("end of input");
        case TOKEN_NAME:
            return str8_lit("name");
        case TOKEN_INT:
            return str8_lit("integer");
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
        case TOKEN_COLON_COLON:
            return str8_lit("colon colon");
        case TOKEN_COLON:
            return str8_lit("colon");
        case TOKEN_ARROW:
            return str8_lit("arrow");
        case TOKEN_COMMA:
            return str8_lit("comma");
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
            return str8_lit("return keyword");
        case TOKEN_KEYWORD_IF:
            return str8_lit("if keyword");

        case TOKEN_INVALID:
            return str8_lit("invalid token");
        default:
            fprintf(stderr, "ERROR: No name defined for token kind: %d\n", kind);
    }

    return (String8){0};
}

bool is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

bool is_ident(char c) {
    return isalnum(c) || c == '_';
}

bool is_number_start(char c) {
    if (isdigit(c)) return true;
    return false;
}

void trim_input_left(Lexer *lexer) {
    while (lexer->input_cursor < lexer->input.size && isspace(lexer->input.str[lexer->input_cursor])) {
        eat_char(lexer);
    }
}

bool lexer_starts_with(Lexer *lexer, String8 prefix) {
    if (prefix.str == 0) {
        return true;
    }
    if (lexer->input_cursor + prefix.size - 1 >= lexer->input.size) {
        return false;
    }
     
    for (size_t i = 0; i < prefix.size; i++) {
        if (prefix.str[i] != lexer->input.str[lexer->input_cursor + i]) {
            return false;
        }
    }

    return true;
}

inline void eat_char(Lexer *lexer) {
    debug_assert(lexer->input_cursor < lexer->input.size);
    char c = lexer->input.str[lexer->input_cursor];
    lexer->input_cursor += 1;
    if (c == '\n') {
        lexer->current_line += 1;
        lexer->current_char = lexer->input_cursor;
    }
}

inline char peek_next_char(Lexer *lexer) {
    return lexer->input.str[lexer->input_cursor + 1];
}

inline Token *get_unused_token(Lexer *lexer) {
    debug_assert(lexer->incoming_tokens_count < MAX_CONCURRENT_TOKENS);

    // Mask off bits so we wrap around if cursor + incoming is bigger than size of array
    u64 index = (lexer->token_array_cursor + lexer->incoming_tokens_count) & (MAX_CONCURRENT_TOKENS - 1);

    Token *result      = &lexer->tokens[index];
    result->kind       = TOKEN_INVALID;
    result->text.size  = 0;
    result->start_line = lexer->current_line;
    result->start_char = lexer->current_char;

    return result;
}

Token *lexer_make_token_literal(Lexer *lexer, TokenLiteral literal) {
    Token *result     = get_unused_token(lexer);
    size_t text_len   = literal.symbol.size; 
    result->kind      = literal.kind; 
    result->text.str  = literal.symbol.str;
    result->text.size = text_len;
    result->end_line  = lexer->current_line;
    result->end_char  = result->start_char + text_len;

    lexer->input_cursor += text_len;
    
    return result;
}

Token *lexer_make_ident_or_keyword(Lexer *lexer) {
    Token *result = get_unused_token(lexer);
    result->text.str = &lexer->input.str[lexer->input_cursor];
    while (lexer->input_cursor < lexer->input.size && is_ident(lexer->input.str[lexer->input_cursor])) {
        eat_char(lexer);
        result->text.size += 1;
    }
    result->kind = TOKEN_NAME;
    
    // Keywords
    TokenKind keyword_kind = is_keyword(result);
    if (keyword_kind) {
        result->kind = keyword_kind;
    }

    result->end_line = lexer->current_line;
    result->end_char = result->start_char + result->text.size;

    return result;
}

Token *lexer_make_eof(Lexer *lexer) {
    Token *result = get_unused_token(lexer);
    result->kind = TOKEN_END;
    result->text = (String8){0};
    return result;
}

Token *lexer_make_string(Lexer *lexer) {
    Token *result = get_unused_token(lexer);
        result->kind = TOKEN_STRING;
        eat_char(lexer); 
        result->text.str = &lexer->input.str[lexer->input_cursor]; 
        while (lexer->input_cursor < lexer->input.size && lexer->input.str[lexer->input_cursor] != '"') {
            eat_char(lexer);
            result->text.size += 1;
        }
        eat_char(lexer); 

        result->end_line = lexer->current_line;
        result->end_char = result->start_char + result->text.size;

        return result;
}

Token *lexer_make_integer(Lexer *lexer) {
    Token *result = get_unused_token(lexer);
    result->kind = TOKEN_INT;
    result->text.str = &lexer->input.str[lexer->input_cursor];
    while (lexer->input_cursor < lexer->input.size && is_number_start(lexer->input.str[lexer->input_cursor])) {
        eat_char(lexer);
        result->text.size += 1;
    }

    result->end_line = lexer->current_line;
    result->end_char = result->start_char + result->text.size;

    return result;
}

Token *lexer_make_invalid(Lexer *lexer) {
    Token *result = get_unused_token(lexer);
    result->kind= TOKEN_INVALID;
    result->text.size = 1;

    return result;
}

Token *peek_token(Lexer *lexer, i64 lookahead_index) {
    debug_assert(lookahead_index < MAX_CONCURRENT_TOKENS);
    debug_assert(lookahead_index >= 0);

    if (lookahead_index == 0) return peek_next_token(lexer);

    while (lexer->incoming_tokens_count <= lookahead_index) {
        make_new_token(lexer);
        lexer->incoming_tokens_count++;
    }

    u64 index = (lexer->token_array_cursor + lookahead_index) & (MAX_CONCURRENT_TOKENS - 1);
    return &lexer->tokens[index];
}

Token *peek_next_token(Lexer *lexer) {
    if (lexer->incoming_tokens_count) {
        debug_assert(lexer->tokens[lexer->token_array_cursor].end_line != -1);
        return &lexer->tokens[lexer->token_array_cursor];
    }

    make_new_token(lexer);
    lexer->incoming_tokens_count++;

    debug_assert(lexer->tokens[lexer->token_array_cursor].end_line != -1);
    return &lexer->tokens[lexer->token_array_cursor];
}

void eat_token(Lexer *lexer) {
    debug_assert(lexer->incoming_tokens_count > 0);
    // debug_assert max tokens is a power of 2
    debug_assert((MAX_CONCURRENT_TOKENS & (MAX_CONCURRENT_TOKENS - 1)) == 0);

    lexer->incoming_tokens_count--;
    // Increment cursor and wrap around if it exceeds last index in token array;
    lexer->token_array_cursor = (lexer->token_array_cursor + 1) & (MAX_CONCURRENT_TOKENS - 1);
}

TokenKind is_keyword(Token *token) {
    if (str8_compare(token->text, str8_lit("return"))) return TOKEN_KEYWORD_RETURN; 
    if (str8_compare(token->text, str8_lit("if")))     return TOKEN_KEYWORD_IF; 
    if (str8_compare(token->text, str8_lit("struct"))) return TOKEN_KEYWORD_STRUCT; 
    if (str8_compare(token->text, str8_lit("else"))) return TOKEN_KEYWORD_ELSE; 

    // TODO: Fix. TokenKind 0 is TOKEN_PLUS which doesn't make sense to return here
    return (TokenKind)0;
}

inline Token *make_new_token(Lexer *lexer) {
    if (!lexer->input.size) return &(Token){0};

    trim_input_left(lexer);
    
    if (lexer->input.str[lexer->input_cursor] == '/') {
        if (peek_next_char(lexer) == '/') {
            size_t comment_line = lexer->current_line;
            while (comment_line == lexer->current_line) {
                eat_char(lexer);
            }
        }
    }
    
    trim_input_left(lexer);
    
    if (lexer->input_cursor >= lexer->input.size) return lexer_make_eof(lexer);
    
    // Strings
    // TODO: This does not work correctly
    if (lexer->input.str[lexer->input_cursor] == '"') {
        return lexer_make_string(lexer);
    }
    
    // Literals
    for (size_t i = 0; i < array_count(token_literals); i++) {
        // If current character is a prefix of a literal, lex the whole literal
        if (lexer_starts_with(lexer, token_literals[i].symbol)) {
            return lexer_make_token_literal(lexer, token_literals[i]);
        }
    }

    // Identifiers and keywords
    if (is_ident_start(lexer->input.str[lexer->input_cursor])) {
        return lexer_make_ident_or_keyword(lexer);
    }

    // Numbers 
    if (is_number_start(lexer->input.str[lexer->input_cursor])) {
        return lexer_make_integer(lexer);
    }

    eat_char(lexer);
    return lexer_make_invalid(lexer);
}
