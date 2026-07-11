typedef enum TokenKind {
    // Literals. These need to ALWAYS be defined the start of the enum, because we have table of token literals,
    // that is defined using these enums values as designated initializers, which we use to then do linear search
    // over the table when we lex literals.
    TOKEN_PLUS,
    TOKEN_ARROW,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DOT,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_CURLY,
    TOKEN_RIGHT_CURLY,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON_COLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_EQUAL_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EXCLAMATION,
    TOKEN_LT_EQUAL,
    TOKEN_GT_EQUAL,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_AND,

    // Uhh misc idk
    TOKEN_END,
    TOKEN_NAME,
    TOKEN_INT,
    TOKEN_STRING,

    //Keywords
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_EXTERN,

    TOKEN_INVALID,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    String8 text;

    i32 start_line;
    i32 end_line;
    
    i32 start_char;
    i32 end_char; 
};

typedef struct TokenLiteral TokenLiteral;
struct TokenLiteral {
    String8 symbol;
    TokenKind kind;
    i32 precedence;
};

TokenLiteral token_literals[];

// This needs to be a power of 2, because we are masking off bits when calculating the index
// into the array, so it acts like a ring buffer.
#define MAX_CONCURRENT_TOKENS 8

// Forward declare because we cant include parser.h (circular include)
struct Parser;

typedef struct Lexer Lexer;
struct Lexer {
    String8 input;
    u64 input_cursor;
    u64 current_line;
    u64 current_char;

    Token tokens[MAX_CONCURRENT_TOKENS];
    u64 token_array_cursor; 
    i64 incoming_tokens_count;

    struct Parser *parser;
};

Token *lexer_make_eof(Lexer *lexer);
Token *lexer_make_invalid(Lexer *lexer);
Token *lexer_make_number(Lexer *lexer);
Token *lexer_make_string(Lexer *lexer);
Token *lexer_make_ident_or_keyword(Lexer *lexer);
Token *lexer_make_token_literal(Lexer *lexer, TokenLiteral literal);

Token *make_new_token(Lexer *lexer);
Token *get_unused_token(Lexer *lexer);
Token *peek_token(Lexer *lexer, i64 lookahead_index);
Token *peek_next_token(Lexer *lexer);
void eat_token(Lexer *lexer);

void eat_char(Lexer *lexer);
char peek_next_char(Lexer *lexer);

String8 token_kind_name(TokenKind kind);
TokenKind is_keyword(Token *token);