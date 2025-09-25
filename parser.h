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

typedef struct Expr Expr;

typedef struct ExprList ExprList;
struct ExprList {
    Expr **items;
    u64 size;
    u64 capacity;
};

typedef enum ExprKind {
    EXPR_NAME,
    EXPR_INT,
    EXPR_CALL,
    EXPR_BINARY,
    EXPR_FIELD,
    EXPR_COMPOUND,
} ExprKind;

struct Expr {
    ExprKind kind;
    union {
        String8 name;
        i64 int_value;
        struct { Expr *left; Expr *right; String8 op; } binary;
        struct { Expr *expr; String8 op; } unary;
        struct { Expr *expr; ExprList params; } call;
        struct { Expr *expr; String8 name; } field;
        // TODO: Support designated initialization 
        struct { ExprList fields; } compound_literal;
    };
};

typedef struct TypeSyntax TypeSyntax;
typedef struct StmtList StmtList;
typedef struct Stmt Stmt;

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

typedef struct Field Field;
struct Field {
    String8 name;
    TypeSyntax *type;
};

typedef struct FieldList FieldList;
struct FieldList {
    Field **items;
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

typedef struct Decl Decl;
struct Decl {
    DeclKind kind;
    String8 name;
    union {
        struct { TypeSyntax *type; Expr *expr; } var;
        struct { ParamList params; TypeSyntax *return_type; StmtList body; } func;
        struct { FieldList fields; } aggregate;
    };
};

typedef struct DeclList DeclList;
struct DeclList {
    Decl **items;
    u64 size;
    u64 capacity;
};
typedef enum StmtKind {
    STMT_NONE,
    STMT_BLOCK,
    STMT_RETURN,
    STMT_IF,
    STMT_ASSIGN,
    STMT_EXPR,
    STMT_DECL,
} StmtKind;

struct Stmt {
    StmtKind kind;
    union {
        Expr* expr;
        Decl *decl;
        StmtList block;
        struct  { String8 name; Expr *expr; } assign_stmt;
        struct { Expr *cond; StmtList if_body; StmtList else_body; } if_stmt;
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
String8 to_operator(Token *token);
b32 is_binary_operator(Token *token);
i32 get_precedence_from_kind(TokenKind kind);

// Make ASTNodes
Expr *make_s32(Arena *arena, Token *token); 
Expr *make_expr_binary(Arena *arena, Expr *left, String8 operator, Expr *right);
Stmt *make_stmt_return(Arena *arena, Expr *expr);
FuncParam *make_decl_param(Arena* arena, String8 name, TypeSyntax *type);
Field *make_field(Arena *arena, String8 name, TypeSyntax* type);
Decl *make_decl_func(Arena *arena, String8 name, ParamList params, StmtList body, TypeSyntax *return_type);
Decl *make_decl_struct(Arena *arena, String8 struct_name, FieldList fields);
Stmt *make_stmt_if(Arena *arena, Expr *cond, StmtList if_body, StmtList else_body);
Expr *make_expr_name(Arena *arena, String8 name);
Expr *make_expr_call(Arena *arena, Expr* expr, ExprList params);
Expr *make_expr_compound(Arena *arena, ExprList fields);
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

// Statement parsing
Stmt *parse_stmt(Lexer *lexer);
StmtList parse_stmts(Lexer *lexer);

Decl *parse_decl_func(Lexer *lexer, String8 name);
FuncParam *parse_decl_param(Lexer *lexer);
ParamList parse_decl_params(Lexer *lexer);
Decl *parse_decl_struct(Lexer *lexer, String8 struct_name);
FieldList parse_aggregate_fields(Lexer *lexer);
Field *parse_aggregate_field(Lexer *lexer);

Stmt *parse_stmt_decl(Lexer *lexer, String8 name);
Stmt *parse_stmt_expr(Lexer *lexer);
Decl *parse_decl_var(Lexer *lexer, String8 name);
Stmt *parse_stmt_assign(Lexer *lexer, String8 name);

TypeSyntax *parse_type_base(Lexer *lexer);

// Keywords parsing
Stmt *parse_stmt_if(Lexer *lexer);
Stmt *parse_stmt_return(Lexer *lexer);

void parser_expect(Lexer *lexer, Token *token, TokenKind expected);
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