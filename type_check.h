#pragma once 
#include "parser.h"

typedef enum TypeKind {
    TYPE_VOID,
    TYPE_S8,
    TYPE_S16,
    TYPE_S32,
    TYPE_S64,
    TYPE_NUMERICAL_END,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_ARRAY,
    TYPE_PTR,
} TypeKind;

typedef struct Symbol Symbol;
typedef struct Field Field;

typedef struct FieldList FieldList;
struct FieldList {
    Field *items;
    u64 size;
    u64 capacity;
};

typedef struct Type Type;
struct Type {
    TypeKind kind;
    u64 size;
    u64 align;

    Symbol *symbol;
    union {
        struct { Type* return_type; Type** params; u64 param_count; } function;
        struct { FieldList fields; } aggregate;
        struct { Type* base; u64 size; } array;
        struct { Type* base; } ptr;
    };
};

typedef struct Field Field;
struct Field {
    String8 name;
    Type *type;
};


typedef enum SymbolKind {
    SYMBOL_NONE,
    SYMBOL_VAR,
    SYMBOL_CONST,
    SYMBOL_FUNC,
    SYMBOL_TYPE,
    SYMBOL_ENUM_CONST,
} SymbolKind;

typedef enum SymbolState {
    SYMBOL_UNRESOLVED,
    SYMBOL_RESOLVING,
    SYMBOL_RESOLVED,
} SymbolState;

struct Symbol {
    String8 name;
    SymbolKind kind;
    SymbolState state;
    Type *type;
    i64 stack_offset;
    Decl *decl; 
    u64 value;
};

static Symbol builtin_symbols[] = {
    [TYPE_VOID] = { .name = str8_lit_comp("void"), .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED, .type = &(Type){.kind = TYPE_VOID } },
    [TYPE_S8]  =  { .name = str8_lit_comp("s8"),   .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED, .type = &(Type){.kind = TYPE_S8,  .size= 1, .align = 1} },
    [TYPE_S16]  = { .name = str8_lit_comp("s16"),  .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED, .type = &(Type){.kind = TYPE_S16, .size= 2, .align = 2} },
    [TYPE_S32]  = { .name = str8_lit_comp("s32"),  .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED, .type = &(Type){.kind = TYPE_S32, .size= 4, .align = 4} },
    [TYPE_S64]  = { .name = str8_lit_comp("s64"),  .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED, .type = &(Type){.kind = TYPE_S64, .size= 8, .align = 8} },
};

typedef struct SymbolList SymbolList;
struct SymbolList {
    Symbol **items;
    u64 size;
    u64 capacity;
};

// https://nullprogram.com/blog/2023/09/30/ 
typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolTable *child[4];
    String8 key;
    Symbol *value;
};

typedef struct TypeSet TypeSet;
struct TypeSet {
    TypeSet *child[4];
    Type *key;
};

// TODO: I should not use scope.arena to allocate symbols in this 
// static SymbolTable *GLOBAL_ORDERED_SYMBOLS = {0};

typedef struct Scope Scope;
struct Scope {
    SymbolTable *symbols;
    Type *expected_return_type;
    Type *expected_decl_type;
    Scope *parent;
    Arena *arena;
};

typedef struct ResolvedExpr {
    Type *type;
    i64 value;
    b32 is_const;
    b32 is_lvalue;
} ResolvedExpr;

static TypeSet *GLOBAL_TYPE_TABLE = {0};
// NOTE: Should i just define primitive types like this?
// static Type *type_int = &(Type){.kind = TYPE_S32, .size= 4, .align = 4};

TypeKind type_kind_from_str8(String8 string);
String8 str8_from_type_kind(TypeKind kind);
String8 name_of_type(Type *type);
String8 expr_kind_name(ExprKind kind);
String8 stmt_kind_name(StmtKind kind);

b32 type_equals(Type* a, Type* b);

Scope *make_child_scope(Arena *local_arena, Scope *parent_scope);

void register_builtin_symbols(Scope *global_scope);
void register_decls(DeclList decls, Scope *scope);
void register_decl(Decl *decl, Scope *scope);

Type *resolve_type_syntax(TypeSyntax *type_syntax, Scope *scope);
Type *resolve_decl_func(Decl *decl, Scope *scope);
ResolvedExpr resolve_expr(Expr *expr, Scope *scope);
i64 resolve_const_expr(Expr *expr, Scope *scope);
Symbol *resolve_name(String8 name, Scope *scope);
void resolve_stmt_block(StmtList block, Scope *scope);
void resolve_stmt(Stmt *stmt, Scope *scope);
void resolve_symbol(Symbol *symbol, Scope *scope);
void resolve_symbols(SymbolTable *symbols, Scope *scope);
void complete_type(Type *type, Scope *scope);

TypeSyntax *make_type_syntax(String8 name, Type *type, TypeSyntax *type_syntax, b32 is_defined);
Type *make_type_function(Type **parameters, u64 parameter_count, Type *return_type);
Type *make_type_array(Type* elem_type, u64 size);
void make_type_struct(Type *type, FieldList fields);
void make_type_union(Type *type, FieldList fields);
Type *make_type_builtin(TypeKind kind);

u64 str8_hash(String8 key);
b32 table_insert(Arena *arena, SymbolTable **table, String8 key, Symbol *value);
Symbol *table_get(SymbolTable *table, String8 key);
Symbol *scope_get_symbol(Scope *scope, String8 key);

Type *type_intern(Arena *arena, TypeSet **set, Type *key);

b32 fields_have_duplicates(FieldList fields);

void symbol_print_decls(SymbolTable *symbols);
void print_type(Type *type);