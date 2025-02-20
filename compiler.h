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

// Strings
u64 CalculateCStringLength(char *cstr);

String8 S8(u8 *str, u64 size);

#define S8CString(s) S8((u8 *)(s), CalculateCStringLength(s))
#define S8Lit(s)     (String8){(u8 *)(s), sizeof(s) - 1}
#define S8LitComp(s) {(u8 *)(s), sizeof(s) - 1}

bool S8Compare(const String8 str1, const String8 str2);

#endif // COMPILER_H