#ifndef PARSER_H 
#define PARSER_H 

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "lexer.h"
#include "base/base_arena.h"

typedef enum ParserContext {
    CONTEXT_DEFAULT,
    CONTEXT_CONDITION,
} ParserContext;

typedef struct Parser Parser;
struct Parser {
    Arena *arena;
    String8 file;
    ParserContext context;
};

typedef struct Symbol Symbol;
typedef struct Expr Expr;
typedef struct TypeSyntax TypeSyntax;
typedef struct StmtList StmtList;
typedef struct Stmt Stmt;

typedef struct ExprList ExprList;
struct ExprList {
    Expr **items;
    u64 size;
    u64 capacity;
};

typedef enum ExprKind {
    EXPR_NAME,
    EXPR_INT_LITERAL,
    EXPR_STRING_LITERAL,
    EXPR_CALL,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_FIELD,
    EXPR_COMPOUND,
} ExprKind;

struct Expr {
    ExprKind kind;
    union {
        String8 name;
        Symbol *symbol;
        i64 int_value;
        String8 string;
        struct { Expr *left; Expr *right; TokenKind op; } binary;
        struct { Expr *expr; TokenKind op; } unary;
        struct { Expr *expr; ExprList params; } call;
        struct { Expr *expr; String8 name; } field;
        // TODO: Support designated initialization 
        struct { ExprList fields; TypeSyntax *type; } compound_literal;
    };
};

struct StmtList {
    Stmt **items;
    u64 size;
    u64 capacity;
};

typedef struct FuncParam FuncParam;
struct FuncParam {
    String8 name;
    TypeSyntax *type;
};

typedef struct ParamList ParamList;
struct ParamList {
    FuncParam **items;
    u64 size;
    u64 capacity;
};

typedef struct FieldSyntax FieldSyntax;
struct FieldSyntax {
    String8 name;
    TypeSyntax *type;
};

typedef struct FieldSyntaxList FieldSyntaxList;
struct FieldSyntaxList {
    FieldSyntax **items;
    u64 size;
    u64 capacity;
};

typedef enum DeclKind {
    DECL_VAR,
    DECL_FUNC,
    DECL_STRUCT,
    DECL_UNION,
    DECL_ENUM,
} DeclKind;

typedef struct Symbol Symbol;

typedef enum Linkage {
    LINKAGE_INTERNAL,
    LINKAGE_EXTERNAL,
} Linkage;

typedef struct Decl Decl;
struct Decl {
    DeclKind kind;
    String8 name;
    Linkage linkage;
    union {
        struct { TypeSyntax *type; Expr *expr; } var;
        struct { ParamList params; TypeSyntax *return_type; StmtList body; } func;
        struct { FieldSyntaxList fields; } aggregate;
    };
    Symbol *symbol;
};

typedef struct DeclList DeclList;
struct DeclList {
    Decl **items;
    u64 size;
    u64 capacity;
};
typedef enum StmtKind {
    // What is the use for STMT_NONE?
    STMT_NONE,
    STMT_BLOCK,
    STMT_RETURN,
    STMT_IF,
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_DECL,
    STMT_WHILE,
} StmtKind;

struct Stmt {
    StmtKind kind;
    union {
        Expr* expr;
        Decl *decl;
        StmtList block;
        struct  { Expr *lhs; Expr *rhs; } assign_stmt;
        struct { Expr *cond; StmtList if_body; StmtList else_body; } if_stmt;
        struct { Expr *cond; StmtList body; } while_stmt;
    };
};

typedef enum TypeSyntaxKind {
    TYPE_SYNTAX_NAME,
    TYPE_SYNTAX_FUNCTION, 
    TYPE_SYNTAX_POINTER,
    TYPE_SYNTAX_ARRAY,
} TypeSyntaxKind;

struct TypeSyntax {
    TypeSyntaxKind kind;

    union {
        String8 name;
        struct { TypeSyntax *return_type; TypeSyntax **params; u64 param_count; } function;
        struct { TypeSyntax *base; Expr *size; } array;
        struct { TypeSyntax *base; } ptr;
    };
};

// Operator parsing helpers
TokenKind to_operator(Token *token);
b32 is_binary_operator(Token *token);
i32 get_precedence_from_kind(TokenKind kind);

// Make ASTNodes
Expr *make_s32(Arena *arena, Token *token); 
Expr *make_expr_binary(Arena *arena, Expr *left, TokenKind operator, Expr *right);
Expr *make_expr_unary(Arena *arena, TokenKind operator, Expr *operand);
Stmt *make_stmt_return(Arena *arena, Expr *expr);
Stmt *make_stmt_while(Arena *arena, Expr *cond, StmtList body);
FuncParam *make_decl_param(Arena* arena, String8 name, TypeSyntax *type);
FieldSyntax *make_field_syntax(Arena *arena, String8 name, TypeSyntax* type);
Decl *make_decl_func(Arena *arena, String8 name, ParamList params, StmtList body, TypeSyntax *return_type, b32 is_external_func);
Decl *make_decl_struct(Arena *arena, String8 struct_name, FieldSyntaxList fields);
Stmt *make_stmt_if(Arena *arena, Expr *cond, StmtList if_body, StmtList else_body);
Expr *make_expr_name(Arena *arena, String8 name);
Expr *make_expr_call(Arena *arena, Expr* expr, ExprList params);
Expr *make_expr_compound(Arena *arena, ExprList fields, TypeSyntax *type);
Decl *make_var_decl(Arena *arena, String8 name, TypeSyntax* type, Expr *expr);
Decl *make_var_assign(Arena *arena, String8 name, Expr *expr);

TypeSyntax *make_type_syntax_name(Arena *arena, String8 name);

DeclList parse_program(Lexer *lexer);

// Expression parsing
Expr *parse_leaf(Lexer *lexer);
Expr *parse_expr(Lexer *lexer, i32 min_precedence);
Expr *parse_increasing_precedence(Lexer *lexer, Expr *left, i32 min_precedence);
Expr *parse_expr_call(Lexer *lexer, Expr *expr);
ExprList parse_call_params(Lexer *lexer);
Expr *parse_expr_compound_field(Lexer *lexer);
Expr *parse_expr_compound(Lexer *lexer, TypeSyntax *type);

// Statement parsing
Stmt *parse_stmt(Lexer *lexer);
StmtList parse_stmts(Lexer *lexer);

Decl *parse_decl_func(Lexer *lexer, String8 name, b32 is_external_function);
FuncParam *parse_decl_param(Lexer *lexer);
ParamList parse_decl_params(Lexer *lexer);
Decl *parse_decl_struct(Lexer *lexer, String8 struct_name);
FieldSyntaxList parse_aggregate_fields(Lexer *lexer);
FieldSyntax *parse_aggregate_field(Lexer *lexer);

Stmt *parse_stmt_decl(Lexer *lexer, String8 name);
Stmt *parse_stmt_expr(Lexer *lexer, Expr *expr);
Decl *parse_decl_var(Lexer *lexer, String8 name);
Stmt *parse_stmt_assign(Lexer *lexer, Expr *lhs);
Stmt *parse_stmt_while(Lexer *lexer);

TypeSyntax *parse_type(Lexer *lexer, TypeSyntax **type);

// Keywords parsing
Stmt *parse_stmt_if(Lexer *lexer);
Stmt *parse_stmt_return(Lexer *lexer);

Token *parser_expect(Lexer *lexer, Token *token, TokenKind expected);
void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...);

// Parse tree printing...
void ast_print(DeclList decls, i32 depth);
void ast_print_decl(Decl *decl, i32 depth);
void ast_print_expr(Expr *expr, i32 depth);
void ast_print_stmt(Stmt *stmt, i32 depth);
void ast_print_type_syntax(TypeSyntax *type_syntax, i32 depth);

// String/Number conversions
i32 i32_from_str8(String8 str);
String8 str8_from_i32(Arena *arena, i32 num);

#endif //PARSER_H