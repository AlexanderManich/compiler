#ifndef PARSER_H 
#define PARSER_H 

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "lexer.h"
#include "base/base_arena.h"

typedef struct Parser Parser;
struct Parser {
    Arena *arena;
};

typedef enum NodeKind {
    NODE_IDENT,
    NODE_PARAMETER,
    NODE_PARAMETER_LIST,
    NODE_VAR_DECL,
    NODE_VAR_ASSIGN,
    NODE_INT,
    NODE_BINARY_OP,
    NODE_RETURN,
    NODE_FUNCTION,
    NODE_IF,
    NODE_BLOCK
} ASTNodeKind;

typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeKind kind;
    union {
        struct { i64 value;} integer;
        struct { String8 name;} indentifier;
        struct { ASTNode *left; ASTNode *right; String8 op;} binary_op;
        struct { ASTNode *expr;} return_val;
        struct { String8 name; String8 type;} parameter;
        struct { ASTNode **items; u64 size; u64 capacity;} parameter_list;
        struct { String8 name; ASTNode* parameters; ASTNode *body;} function_decl;
        struct { ASTNode **items; u64 size; u64 capacity;} block;
        struct { ASTNode *cond; ASTNode *if_body; ASTNode *else_body;} _if;
        struct { String8 name; ASTNode *expr;} var_assign;
        struct { String8 name; ASTNode *expr;} var_decl;
    };
};

String8 node_kind_name(ASTNode* node);

// Operator parsing helpers
String8 to_operator(Token *token);
b32 is_binary_operator(Token *token);
i32 get_precedence_from_kind(TokenKind kind);

// Make ASTNodes
ASTNode *make_number(Arena *arena, Token *token); 
ASTNode *make_binary(Arena *arena, ASTNode *left, String8 operator, ASTNode *right);
ASTNode *make_return(Arena *arena, ASTNode *expr);
ASTNode *make_parameter(Arena* arena, String8 name, String8 type);
// ASTNode *make_parameters(Arena *arena, String8 name, String8 type);
ASTNode *make_function_decl(Arena *arena, String8 name, ASTNode* parameters, ASTNode *body);
ASTNode *make_if(Arena *arena, ASTNode *cond, ASTNode *if_body, ASTNode *else_body);
ASTNode *make_indentifier(Arena *arena, Token *token);
ASTNode *make_var_decl(Arena *arena, String8 name, ASTNode *expr);
ASTNode *make_var_assign(Arena *arena, String8 name, ASTNode *expr);

ASTNode *parse_program(Lexer *lexer);

// Expression parsing
ASTNode *parse_leaf(Lexer *lexer);
ASTNode *parse_expression(Lexer *lexer, i32 min_precedence);
ASTNode *parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence);

// Statement parsing
ASTNode *parse_statement(Lexer *lexer);
ASTNode *parse_statements(Lexer *lexer);

ASTNode* parse_function_decl(Lexer *lexer, String8 name);

ASTNode* parse_var_decl(Lexer *lexer, String8 name);
ASTNode* parse_var_assign(Lexer *lexer, String8 name);

// Keywords parsing
ASTNode *parse_if(Lexer *lexer);
ASTNode *parse_return(Lexer *lexer);

void parser_expect(Lexer *lexer, Token *token, TokenKind expected);
void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...);

// Parse tree printing...
typedef struct CharGrid CharGrid;
struct CharGrid {
    u8 *grid;
    u32 height;
    u32 width;
};

u64 calc_ast_tree_depth(ASTNode *tree);
void print_ast(Arena *arena, ASTNode *tree);
CharGrid _print_ast(Arena *arena, CharGrid buffer, ASTNode *tree, u32 x, u32 y);

// String/Number conversions
i32 i32_from_str8(String8 str);
String8 str8_from_i32(Arena *arena, i32 num);

#endif //PARSER_H