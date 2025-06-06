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
    Type* return_type;
    Type** params;
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

typedef struct TypeSet TypeSet;
struct TypeSet {
    TypeSet *child[4];
    Type* key;
};

typedef struct Scope Scope;
struct Scope {
    SymbolTable *symbols;
    TypeSet *types;
    Type *expected_return_type;
    Scope *parent;
    Arena *arena;
};

typedef struct ASTNode ASTNode;

TypeKind type_kind_from_str8(String8 string);
String8 str8_from_type_kind(TypeKind kind);

Type simple_type(TypeKind kind);
Type *make_simple_type(Arena *arena, TypeSet *type_set, TypeKind kind);
Type *make_function_type(Arena *arena, TypeSet *type_set, ASTNode *node);
Type *make_expr_type(Arena *arena, TypeSet *type_set, ASTNode *expr);

b32 type_equals(Type* a, Type* b);

Scope *make_child_scope(Scope *parent_scope);

void collect_declarations(ASTNode *node, Scope *scope);
void type_check_program(ASTNode *root_node, Scope* global_scope);
void type_check_node(ASTNode *node, Scope *scope);
Type *type_resolve(String8 type_name, Scope *scope);

u64 str8_hash(String8 key);
b32 table_insert(Arena *arena, SymbolTable **table, String8 key, Symbol value);
Symbol *table_get(SymbolTable *table, String8 key);

Type *type_intern(Arena *arena, TypeSet **set, Type *key);