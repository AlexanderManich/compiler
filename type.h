typedef struct Symbol Symbol;
typedef struct Type Type;

typedef enum AdressingMode {
    ADRESSING_INVALID,
    ADRESSING_NO_VALUE,
    ADRESSING_VALUE,
    ADRESSING_VARIABLE,
    ADRESSING_CONSTANT,
    ADRESSING_TYPE,
} AdressingMode;

typedef struct TypeAndValue {
    AdressingMode mode;
    Type *type;
    Value value;
} TypeAndValue;

typedef enum TypeKind {
    TYPE_VOID,
    TYPE_S8,
    TYPE_S16,
    TYPE_S32,
    TYPE_S64,
    TYPE_STRING,
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

typedef struct Field {
    String8 name;
    Type *type;
} Field;

typedef struct FieldList {
    Field *items;
    u64 size;
    u64 capacity;
} FieldList;

typedef struct Type {
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
} Type;

TypeAndValue tav_lvalue(Type *type);
TypeAndValue tav_rvalue(Type *type);
TypeAndValue tav_const(Type *type, Value value);
TypeAndValue tav_novalue(void);