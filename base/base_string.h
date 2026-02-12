#ifndef BASE_STRING_H
#define BASE_STRING_H

#include "base_include.h"

typedef struct String8 String8;
struct String8 {
    u8 *str;
    u64 size;
};

typedef struct String8Array {
    String8 *buf;
    u64 count;
    u64 total_size;
} String8Array;

u64 calc_cstring_length(char *cstr);

String8 str8(u8 *str, u64 size);

#define str8_cstring(s)  str8((u8 *)(s), calc_cstring_length(s))
#define str8_lit(s)      (String8){(u8 *)(s), sizeof(s) - 1}
#define str8_lit_comp(s) {(u8 *)(s), sizeof(s) - 1}
#define str8_varg(s)     (int)((s).size), ((s).str)

b32 str8_compare(const String8 str1, const String8 str2);

String8 str8_last_slash_prefix(String8 string);
String8 str8_last_slash_suffix(String8 string);
String8 str8_strip_ext(String8 string);

String8 str8_copy(Arena *arena, String8 str);
String8 str8_concat(Arena *arena, String8 str1, String8 str2);


String8Array str8_array_zero(void);
String8Array str8_array_reserve(Arena *arena, u64 count);


static inline String8 str8_shift_args(char ***argv, int *argc) {
    char *arg_cstr = shift_args(argv, argc);
    return str8_cstring(arg_cstr);
}

#endif // BASE_STRING_H