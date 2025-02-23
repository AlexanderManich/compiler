#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include <stdbool.h>

// Basic types
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

// String types
typedef struct String8 String8;
struct String8 {
    u8 *str;
    u64 size;
};

typedef struct CharGrid CharGrid;
struct CharGrid {
    u8 *grid;
    u32 height;
    u32 width;
};

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// Strings
u64 calc_cstring_length(char *cstr);

String8 str8(u8 *str, u64 size);

#define str8_cstring(s) str8((u8 *)(s), calc_cstring_length(s))
#define str8_lit(s)     (String8){(u8 *)(s), sizeof(s) - 1}
#define str8_lit_comp(s) {(u8 *)(s), sizeof(s) - 1}

bool str8_compare(const String8 str1, const String8 str2);

#endif // COMPILER_H