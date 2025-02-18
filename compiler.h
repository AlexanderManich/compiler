#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>

// Types
typedef struct String8 String8;
struct String8 {
    char *str;
    size_t size;
};

// Strings
size_t CalculateCStringLength(char *cstr);

String8 S8(char *str, size_t size);

#define S8Lit(s)        (String8){(s), sizeof(s) - 1}
#define S8CString(s) S8((s), CalculateCStringLength(s))

#endif // COMPILER_H