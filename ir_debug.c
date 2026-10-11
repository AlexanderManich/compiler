// ir_debug.c
//
// Human-readable dump of the IR built by IRBuilder2, for sanity-checking
// basic block structure, instruction operands, and control flow (including
// implicit fallthrough edges) during development.
//
// Adjust the field accesses marked with NOTE: if your actual struct
// definitions differ (e.g. Value's internal fields, symbol naming, etc).

#include <stdio.h>

// ---- Instruction kind names -------------------------------------------

static const char *instr_kind_name(InstructionKind kind) {
    switch (kind) {
        case IR2_NOOP:           return "noop";
        case IR2_ADD:            return "add";
        case IR2_SUB:            return "sub";
        case IR2_MULT:           return "mult";
        case IR2_LT:             return "lt";
        case IR2_GT:             return "gt";
        case IR2_WRITE:          return "write";
        case IR2_ARG:            return "arg";
        case IR2_CALL:           return "call";
        case IR2_JUMP:           return "jump";
        case IR2_JUMP_IF_FALSE:  return "jump_if_false";
        case IR2_RET:            return "ret";
        // NOTE: add any InstructionKind values not listed above.
        default:                 return "???";
    }
}

// ---- Operand formatting -------------------------------------------------

static void print_operand(IROperand2 op) {
    switch (op.kind) {
        case OP_SYM2:
            // NOTE: assumes symbol has a `name` field; adjust if it
            // stores something else (e.g. an index into a symbol table).
            printf("%*.s", str8_varg(((Symbol *)op.symbol)->name));
            break;
        case OP_TEMP2:
            printf("t%u", op.temp);
            break;
        case OP_LABEL:
            printf(".L%u", op.label);
            break;
        case OP_CONST2:
            // NOTE: adjust for your Value's actual variant tags/fields.
            switch (op.constant.kind) {
                case VALUE_INT:
                    printf("%d", op.constant.integer_value);
                    break;
                default:
                    printf("<const>");
                    break;
            }
            break;
        default:
            printf("<none>");
            break;
    }
}

static void print_instruction(Instruction instr) {
    printf("    %-16s ", instr_kind_name(instr.kind));

    b32 has_result = instr.result.kind != 0 || instr.kind == IR2_WRITE;
    if (has_result) {
        print_operand(instr.result);
        printf(" <- ");
    }

    print_operand(instr.arg1);

    // Only print arg2 if it was actually set (avoid noisy "<none>" spam
    // for unary/no-arg instructions).
    if (instr.arg2.kind != 0) {
        printf(", ");
        print_operand(instr.arg2);
    }

    printf("\n");
}

// ---- Block / function dumping ------------------------------------------

static void print_bblock(BBlock *block, BBlock *next_in_order) {
    printf("  .L%u:\n", block->label);

    if (block->instructions.size == 0) {
        printf("    (empty)\n");
    }

    for (u64 i = 0; i < block->instructions.size; i++) {
        print_instruction(block->instructions.items[i]);
    }

    // Report whether this block ends in an explicit terminator, or is
    // relying on fallthrough to the next block in emission order. This is
    // the case worth double-checking after adding implicit fallthrough
    // into if-bodies: a block with no jump/ret at the end MUST be
    // immediately followed by its intended fallthrough target.
    b32 has_terminator = false;
    if (block->instructions.size > 0) {
        InstructionKind last_kind =
            block->instructions.items[block->instructions.size - 1].kind;
        has_terminator = (last_kind == IR2_JUMP ||
                           last_kind == IR2_JUMP_IF_FALSE ||
                           last_kind == IR2_RET);
    }

    if (!has_terminator) {
        if (next_in_order) {
            printf("    -- falls through to .L%u --\n", next_in_order->label);
        } else {
            printf("    !! WARNING: block has no terminator and no next block !!\n");
        }
    }

    printf("\n");
}

void print_ir_function(IRFunction2 *func, const char *name) {
    printf("function %s:\n", name ? name : "<unnamed>");

    for (u64 i = 0; i < func->blocks.size; i++) {
        BBlock *block = func->blocks.items[i];
        BBlock *next = (i + 1 < func->blocks.size) ? func->blocks.items[i + 1] : NULL;
        print_bblock(block, next);
    }

    printf("\n");
}

void print_ir_program(IRFunctionList2 *program) {
    for (u64 i = 0; i < program->size; i++) {
        // NOTE: swap in the real name field if IRFunction2 stores one
        // (e.g. a Decl* or symbol pointer for the function being built).
        print_ir_function(&program->items[i], NULL);
    }
}