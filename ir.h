#pragma once

#include "base/base_string.h"
typedef enum Op {
    IR_RET,
    IR_MOV,
    IR_LOAD_CONST,
    IR_ADD,
} Op;

typedef struct Instruction Instruction;
struct Instruction {
    Op op;
    i32 src1;
    i32 src2;
    i32 dest;
    i32 imm;
};

#define IR_MAX 1024
extern Instruction ir_code[IR_MAX];
extern i32 ir_count;

void emit(Op op, i32 dest, i32 src, i32 src2, i32 imm);

i32 ir_gen_expr(ASTNode *node);
void ir_gen_statement(ASTNode *node);
void ir_gen_function(ASTNode *func_node);