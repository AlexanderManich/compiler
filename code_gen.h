#pragma once

#include "parser.h"

typedef enum OperandKind {
    OP_REG,
    OP_STACK,
    OP_IMM,
} OperandKind;

typedef struct Operand {
    OperandKind kind;
    i64 value;
} Operand;

typedef enum InstrKind {
    INSTR_LOAD,
    INSTR_LOAD_IMM,
    INSTR_STORE,
    INSTR_STORE_IMM,
    INSTR_COPY,
    INSTR_ADD,
} InstrKind;

typedef struct Instr {
    InstrKind kind;
    Operand    src;
    Operand   dest;
} Instr;

// NOTE: ORDER MATTERS!!! DONT CHANGE
typedef enum Regs64 {
    REG_RAX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
} Regs64;

typedef struct Register {
    String8 name;
    b32   in_use;
} Register;

void codegen_ast(Arena* arena, SymbolList root, String8 output_path);
void asm_compile_and_link(Arena *arena, String8 out_flag);
void emit_instr(InstrKind instr, Operand src, Operand dest);
Operand make_operand(OperandKind kind, i64 value);