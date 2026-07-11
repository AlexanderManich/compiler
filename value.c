// String/Number conversions
// i32 i32_from_str8(String8 str);
// String8 str8_from_i32(Arena *arena, i32 num);

i32 i32_from_str8(String8 str) {
    i32 result = 0;
    u64 i = 0;

    while (str.str[i] >= '0' && str.str[i] <= '9') {
        result = result * 10 + (str.str[i] - '0');
        i++;
    }

    if (i < str.size) {
        fprintf(stderr, "Error: Invalid integer format: %.*s\n", str8_varg(str));
        return 0;
    }

    return result;
}

String8 str8_from_i32(Arena *arena, i32 num) {
    String8 str = {0};

    if (num == 0) {
        return str;
    }

    u32 temp = num;
    u32 length = 0;

    while (temp > 0) {
        temp /= 10;
        length++;
    }

    str.str = push_array(arena, u8, length);
    str.size = length;

    while (num > 0) {
        str.str[--length] = (num % 10) + '0';  
        num /= 10;
    }

    return str;
}

Value make_value_int_from_str8(String8 string) {
    // TODO: This needs to be BigInt! We only support signed 32bit int for now
    Value result = { .kind = VALUE_INT, .integer_value = i32_from_str8(string)};
    return result;
}

Value make_value_str8(String8 string) {
    Value result = { .kind = VALUE_STRING, .string_value = string };
    return result;
}

Value value_from_literal(TokenKind kind, String8 string) {
    switch(kind) {
        case TOKEN_INT:    return make_value_int_from_str8(string);
        case TOKEN_STRING: return make_value_str8(string);
        default: {
            Value result = { .kind = VALUE_INVALID };
            return result;
        }
    }
}

Value value_from_token(Token *token) {
    String8 string = token->text;
    Value value = value_from_literal(token->kind, string);
    if (value.kind == VALUE_INVALID) {
        // TODO: Report syntax errors. Print for now
        switch (token->kind) {
        case TOKEN_INT:
            eprintf("Invalid integer literal");
            break;
        case TOKEN_STRING:
            eprintf("Invalid string literal");
            break;
        default:
            eprintf("Invalid token literal");
            break;
        }
    }
    return value;
}