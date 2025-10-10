#pragma once

#include "base/base_include.h"

typedef struct Expr Expr;

typedef enum TypeSyntaxKind {
    TYPE_SYNTAX_NAME,
    TYPE_SYNTAX_FUNCTION, 
    TYPE_SYNTAX_POINTER,
    TYPE_SYNTAX_ARRAY,
} TypeSyntaxKind;

typedef struct TypeSyntax TypeSyntax;
struct TypeSyntax {
    TypeSyntaxKind kind;

    union {
        String8 name;
        struct { TypeSyntax *return_type; TypeSyntax **params; u64 param_count; } function;
        struct { TypeSyntax *base; Expr *size; } array;
        struct { TypeSyntax *base; } ptr;
    };
};

typedef struct Type Type;


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
typedef struct TypeInfo TypeInfo;

typedef struct Field Field;
struct Field {
    String8 name;
    TypeInfo *type;
};

typedef struct FieldList FieldList;
struct FieldList {
    Field **items;
    u64 size;
    u64 capacity;
};

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

struct TypeInfo {
    TypeSyntax* syntax;
    Type *type;
};
