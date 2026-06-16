typedef enum ValueKind {
    VALUE_INVALID,
    VALUE_INT,
    VALUE_STRING,
} ValueKind;

typedef struct Value {
    ValueKind kind;
    union {
        i32 integer_value; // TODO: This needs to be BigInt
        String8 string_value;
    };
} Value;

// String/Number conversions
// i32 i32_from_str8(String8 str);
// String8 str8_from_i32(Arena *arena, i32 num);

i32 i32_from_str8(String8 str);
String8 str8_from_i32(Arena *arena, i32 num);
Value make_value_int_from_str8(String8 string);
Value make_value_str8(String8 string);
Value value_from_literal(TokenKind kind, String8 string);
Value value_from_token(Token *token);