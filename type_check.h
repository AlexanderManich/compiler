#pragma once 

typedef enum TypeKind {
    TYPE_VOID,
    TYPE_S8,
    TYPE_S16,
    TYPE_S32,
    TYPE_S64,
    TYPE_FUNCTION
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    struct Type* return_type;
    struct Type** params;
    u64 params_count;
};

typedef struct Symbol Symbol;
struct Symbol {
    String8 identifier;
    Type *type;
    b32 is_defined;
};

// https://nullprogram.com/blog/2023/09/30/ 
typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolTable *child[4];
    String8 key;
    Symbol value;
};

typedef struct Scope Scope;
struct Scope {
    SymbolTable *symbols;
    Type *expected_return_type;
    Scope *parent;
    Arena *arena;
};

typedef struct ASTNode ASTNode;

TypeKind type_kind_from_str8(String8 string);
Type *make_simple_type(Arena *arena, TypeKind);
Type *make_function_type(Arena *arena, ASTNode *node);
Type *typecheck_expr(ASTNode *expr, Scope *scope);
b32 type_equal(Type* a, Type* b);

Scope *make_child_scope(Scope *parent_scope);

void collect_declarations(ASTNode *node, Scope *scope);
void type_check_program(ASTNode *root_node, Scope* global_scope);
void type_check_node(ASTNode *node, Scope* scope);

u64 hash(String8 key);
b32 equals(String8 a, String8 b);
b32 map_insert(Arena *arena, SymbolTable **table, String8 key, Symbol value);
Symbol *map_get(SymbolTable *table, String8 key);
