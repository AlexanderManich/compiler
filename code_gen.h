#pragma once

#include "parser.h"

typedef enum OperandKind {
    OP_REG,
    OP_STACK,
    OP_IMM,
    OP_LABEL,
    OP_SYMBOL,
} OperandKind;

typedef struct Operand {
    OperandKind kind;
    u8 size;
    union {
        u64 vreg;
        i64 imm;
        u64 stack_slot;
        void *symbol;
        u32 label_id;
    };
} Operand;

typedef enum InstrKind {
    INSTR_LOAD,
    INSTR_STORE,
    INSTR_COPY,
    INSTR_ADD,
    INSTR_LEA,
} InstrKind;

typedef struct Instr {
    InstrKind kind;
    Operand    src;
    Operand   dest;
} Instr;

// NOTE: ORDER MATTERS!
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

typedef i32 RegHandle;

void codegen_ast(Arena* arena, SymbolList root, String8 output_path);
RegHandle codegen_expr(Expr *expr, i32 force_reg);
void emit_instr(InstrKind instr, Operand src, Operand dest);
static inline Operand op_reg(i32 id, u8 size);
static inline Operand op_stack(i64 offset, u8 size);
static inline Operand op_imm(i64 value, u8 size);
static inline Operand op_label_addr(void *symbol);
static inline Operand make_operand(Expr *expr);