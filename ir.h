#include "base_string.h"
typedef enum Op {
    
} Op;

typedef struct Instruction Instruction;
struct Instruction {
    Op op;
    i32 src1;
    i32 src2;
    i32 dest;
    i32 imm;
};