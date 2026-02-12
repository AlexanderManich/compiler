#include "base_string.h"

u64 calc_cstring_length(char *cstr) {
    u64 i = 0;
    for(; cstr[i]; i += 1);
    return i;
}

String8 str8(u8 *str, u64 size) {
    String8 string = {0};
    string.str  = str;
    string.size = size;
    return string;
}

inline
b32 str8_compare(String8 str1, String8 str2) {
    b32 result = 0;

    if (str1.size == str2.size) {
        result = 1;

        for (u64 i = 0; i < str1.size && i < str2.size; i++) {
            b32 match = (str1.str[i] == str2.str[i]);

            if (!match) {
                result = 0;
                break;
            }
        }
    }

    return result;
}

String8 str8_last_slash_prefix(String8 string) {
    if (string.size > 0) {
        u8 *ptr = string.str + string.size - 1;
        for (; ptr >= string.str; ptr--) {
            if(*ptr == '\\') break;
        }
        if (ptr >= string.str) {
            string.size = (u64)(ptr - string.str);
        }
        else {
            string.size = 0;
        }
    }
    return string;
}

String8 str8_last_slash_suffix(String8 string) {
    if (string.size > 0) {
        u8 *ptr = string.str + string.size - 1;
        for (; ptr >= string.str; ptr--) {
            if(*ptr == '\\') break;
        }
        if (ptr >= string.str) {
            ptr += 1;
            string.size = (u64)(string.str + string.size - ptr);
            string.str = ptr;
        }
    }
    return string;
}

String8 str8_strip_ext(String8 string) {
    if (string.size > 0) {
        u8 *ptr = string.str + string.size - 1;
        for (; ptr >= string.str; ptr--) {
            if (*ptr == '.') break;
        }
        if (ptr >= string.str) {
            string.size = (u64)(ptr - string.str);
        }
    }
    return string;
}

String8 str8_copy(Arena *arena, String8 str) {
    String8 result;
    result.size = str.size;
    result.str = push_array_no_zero(arena, u8, result.size + 1);
    memcpy(result.str, str.str, str.size);
    result.str[str.size] = 0;
    return result;
}

String8 str8_concat(Arena *arena, String8 str1, String8 str2) {
    String8 result;
    result.size = str1.size + str2.size;
    result.str = push_array_no_zero(arena, u8, result.size + 1);
    memcpy(result.str, str1.str, str1.size);
    memcpy(result.str + str1.size, str2.str, str2.size);
    result.str[result.size] = 0;
    return result;
}

String8Array str8_array_zero() {
    String8Array result = {0};
    return result;
}

String8Array str8_array_reserve(Arena *arena, u64 count) {
    String8Array array = {0};
    array.buf = push_array(arena, String8, count);
    return array;
}
