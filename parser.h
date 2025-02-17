#include <stddef.h>
#include <stdio.h>
typedef enum {
    NODE_VAR,
    NODE_INT_LITERAL,
    NODE_BINARY_OP,
} NodeKind;

typedef struct ASTNode ASTNode;
struct ASTNode {
    NodeKind kind;
    union {
        struct { char *name; } var;
        struct { int value; } int_literal;
        struct { ASTNode *left; ASTNode *right; char op; } binary_op;
    };
};

ASTNode *parse_factor();

int string_to_int(const char *str, size_t size) {
    int result = 0;
    size_t i = 0;

    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    if (i < size) {
        fprintf(stderr, "Error: Invalid integer format: &s\n");
        return 0;
    }

    return result;
}
