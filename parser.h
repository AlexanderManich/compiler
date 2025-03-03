#ifndef PARSER_H 
#define PARSER_H 

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "arena.h"

typedef struct Parser Parser;
struct Parser {
    Arena arena;
};

typedef enum NodeKind {
    NODE_VAR,
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
        struct ASTVar { String8 name; } var;
        struct { i32 value; } integer;
        struct { ASTNode *left; ASTNode *right; String8 op; } binary_op;
        struct { ASTNode *expr;} return_val;
        struct { String8 name; ASTNode *body;} function;
        struct { ASTNode *cond; ASTNode *if_body; ASTNode *else_body;} _if;
        struct { struct ASTVar var; ASTNode *expr;} var_assign;
        struct { ASTNode* next; } block;
    };
};

String8 node_kind_name(ASTNode* node);

// Operator parsing helpers
String8 to_operator(Token *token);
bool is_binary_operator(Token *token);
i32 get_precedence(Token *token);

// Make ASTNodes
ASTNode *make_number(Arena *arena, Token *token); 
ASTNode *make_binary(Arena *arena, ASTNode *left, String8 operator, ASTNode *right);
ASTNode *make_return(Arena *arena, ASTNode *expr);
ASTNode *make_function(Arena *arena, String8 name, ASTNode *body);
ASTNode *make_if(Arena *arena, ASTNode *cond, ASTNode *if_body, ASTNode *else_body);

// Expression parsing
ASTNode *parse_leaf(Lexer *lexer);
ASTNode *parse_expression(Lexer *lexer, i32 min_precedence);
ASTNode *parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence);

// Statement parsing
ASTNode *parse_statement(Lexer *lexer);
ASTNode *parse_statements(Lexer *lexer);

ASTNode *parse_function(Lexer *lexer);

// Keywords parsing
ASTNode *parse_if(Lexer *lexer);
ASTNode *parse_return(Lexer *lexer);

void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...);

u64 calc_ast_tree_depth(ASTNode *tree);
void print_ast(ASTNode *tree);
CharGrid _print_ast(CharGrid buffer, ASTNode *tree, u32 x, u32 y);

i32 i32_from_str8(String8 str);
String8 str8_from_i32(i32 num);

#endif //PARSER_H