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
        struct { i32 value;} integer;
        struct { String8 name;} variable;
        struct { ASTNode *left; ASTNode *right; String8 op;} binary_op;
        struct { ASTNode *expr;} return_val;
        struct { String8 name; ASTNode *body;} function;
        struct { ASTNode **statements; u64 size; u64 capacity;} block;
        struct { ASTNode *cond; ASTNode *if_body; ASTNode *else_body;} _if;
        struct { String8 name; ASTNode *expr;} var_assign;
        struct { String8 name; ASTNode *expr;} var_decl;
    };
};

// #define BLOCK_DA_INIT_CAP 256
// #define block_da_append(arena, da, statement)                                                          \
//     do {                                                                                 \
//         if ((da)->block.size >= (da)->block.capacity) {                                             \
//             (da)->block.capacity = (da)->block.capacity == 0 ? BLOCK_DA_INIT_CAP : (da)->block.capacity*2;   \
//             (da)->block.statements = push_array((arena), ASTNode, (da)->block.capacity*sizeof(*(da)->block.statements)); \
//         }                                                                                \
//                                                                                          \
//         (da)->block.statements[(da)->block.size++] = (statement);                                             \
//     } while (0)

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
ASTNode *make_variable(Arena *arena, Token *token);
ASTNode *make_var_decl(Arena *arena, String8 name, ASTNode *expr);
ASTNode *make_var_assign(Arena *arena, String8 name, ASTNode *expr);

// Expression parsing
ASTNode *parse_leaf(Lexer *lexer);
ASTNode *parse_expression(Lexer *lexer, i32 min_precedence);
ASTNode *parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence);

// Statement parsing
ASTNode *parse_statement(Lexer *lexer);
ASTNode *parse_statements(Lexer *lexer);

ASTNode* parse_function(Lexer *lexer);

ASTNode* parse_var_decl(Lexer *lexer, String8 name);
ASTNode* parse_var_assign(Lexer *lexer, String8 name);

// Keywords parsing
ASTNode *parse_if(Lexer *lexer);
ASTNode *parse_return(Lexer *lexer);

void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...);


u64 calc_ast_tree_depth(ASTNode *tree);
void print_ast(Arena *arena, ASTNode *tree);
CharGrid _print_ast(Arena *arena, CharGrid buffer, ASTNode *tree, u32 x, u32 y);

i32 i32_from_str8(String8 str);
String8 str8_from_i32(Arena *arena, i32 num);

#endif //PARSER_H