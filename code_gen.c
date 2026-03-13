#include "parser.h"
#include "code_gen.h"
#include "type_check.h"

FILE *GLOBAL_ASM_HANDLE = 0;

i64 layout_function(Symbol *symbol) {
    debug_assert(symbol->kind == SYMBOL_FUNC);

    i64 offset = 0;
    
    for (u64 i = 0; i < symbol->func.params_count; i++) {
        Symbol *param = symbol->func.params[i];
        offset += align_pow2(param->type->size, 8);
        param->stack_offset = offset;
    }

    for (u64 i = 0; i < symbol->decl->func.body.size; i++) {
        Stmt *stmt = symbol->decl->func.body.items[i];
        if (stmt->kind == STMT_DECL) {
            Symbol *local = stmt->decl->symbol;
            offset += align_pow2(local->type->size, 8);
            local->stack_offset = offset;
        }
    }

    return offset;
}

String8 param_regs32[] = {str8_lit_comp("%edi"), str8_lit_comp("%esi"), str8_lit_comp("%edx"), str8_lit_comp("%ecx"), str8_lit_comp("%r8d"), str8_lit_comp("%r9d")};
Regs64 param_regs64[] = {REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};


#define REGISTERS_COUNT 16
static Register regs[REGISTERS_COUNT] = {
    [REG_RAX] = { .name = str8_lit_comp("%rax"), .in_use = 0 },
    [REG_RCX] = { .name = str8_lit_comp("%rcx"), .in_use = 0 },
    [REG_RDX] = { .name = str8_lit_comp("%rdx"), .in_use = 0 },
    [REG_RSI] = { .name = str8_lit_comp("%rsi"), .in_use = 0 },
    [REG_RDI] = { .name = str8_lit_comp("%rdi"), .in_use = 0 },
    [REG_R8]  = { .name = str8_lit_comp("%r8"),  .in_use = 0 },
    [REG_R9]  = { .name = str8_lit_comp("%r9"),  .in_use = 0 },
    [REG_R10] = { .name = str8_lit_comp("%r10"), .in_use = 0 },
    [REG_R11] = { .name = str8_lit_comp("%r11"), .in_use = 0 },
    [REG_RBX] = { .name = str8_lit_comp("%rbx"), .in_use = 0 },
    [REG_RSP] = { .name = str8_lit_comp("%rsp"), .in_use = 0 },
    [REG_RBP] = { .name = str8_lit_comp("%rbp"), .in_use = 0 },
    [REG_R12] = { .name = str8_lit_comp("%r12"), .in_use = 0 },
    [REG_R13] = { .name = str8_lit_comp("%r13"), .in_use = 0 },
    [REG_R14] = { .name = str8_lit_comp("%r14"), .in_use = 0 },
    [REG_R15] = { .name = str8_lit_comp("%r15"), .in_use = 0 },
};

#define SCRATCH_REG_BEGIN 0
#define SCRATCH_REG_END   8 

i32 alloc_reg(i32 force_reg) {
    i32 result = -1;

    for (u32 i = SCRATCH_REG_BEGIN; i < SCRATCH_REG_END; i++) {
        if (!regs[i].in_use) {
            regs[i].in_use = true;
            result = i;

            if (force_reg != -1) {
                if (regs[force_reg].in_use) {
                    // TODO: How do we determine the size here?
                    emit_instr(INSTR_COPY, op_reg(i, 8), op_reg(force_reg, 8));
                }
                regs[force_reg].in_use = true;
                result = force_reg;
            }
            break;
        };
    }

    return result;
}

void free_reg(i32 reg) {
    regs[reg].in_use = false;
    return;
}

void free_param_regs(void) {
    for (u32 i = 0; i < array_count(param_regs64); i++) {
        free_reg(param_regs64[i]);
    }
    return;
}

String8 reg_name(i32 reg) {
    return regs[reg].name;
}

static u32 LABEL_COUNT = 0;

u32 label_create(void) {
    u32 label = LABEL_COUNT;
    LABEL_COUNT++;
    return label;
}

String8 label_name(Arena *arena, u32 label_id) {
    String8 result = {0};
    result.size = 3;
    result.str = push_array_no_zero(arena, u8, result.size + 1);
    snprintf((char *)result.str, result.size + 1, ".L%u", label_id);
    return result;
}

void emit_instr(InstrKind instr, Operand src, Operand dest) {
    switch (instr) {
        case INSTR_LOAD: {
            debug_assert(dest.kind == OP_REG);
            if (src.kind == OP_STACK)    fprintf(GLOBAL_ASM_HANDLE, "    movq -%ld(%%rbp), %s\n", src.stack_slot, reg_name(dest.vreg).str);
            else if (src.kind == OP_IMM) fprintf(GLOBAL_ASM_HANDLE, "    movq $%ld, %s\n", src.imm, reg_name(dest.vreg).str);
            break;
        }
        case INSTR_STORE: {
            debug_assert(dest.kind == OP_STACK);
            if (src.kind == OP_REG)      fprintf(GLOBAL_ASM_HANDLE, "    movq %s, -%ld(%%rbp)\n", reg_name(src.vreg).str, dest.stack_slot);
            else if (src.kind == OP_IMM) fprintf(GLOBAL_ASM_HANDLE, "    movq $%ld, -%ld(%%rbp)\n", src.vreg, dest.stack_slot);
            break;
        }
        case INSTR_COPY: {
            debug_assert(src.kind == OP_REG);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    movq %s, %s\n", reg_name(src.vreg).str, reg_name(dest.vreg).str);
            break;
        }
        case INSTR_ADD: {
            debug_assert(src.kind == OP_REG);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    addq %s, %s\n", reg_name(src.vreg).str, reg_name(dest.vreg).str);
            break;
        }
        case INSTR_LEA: {
            debug_assert(src.kind == OP_LABEL);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    leaq str_%p(%%rip), %s\n", (void *)src.symbol, reg_name(dest.vreg).str) ;
            break;
        }
    }
}

b32 function_has_call(Symbol *symbol) {
    b32 result = false;
    for (u64 i = 0; i < symbol->decl->func.body.size; i++) {
        Stmt *stmt = symbol->decl->func.body.items[i];
        if (stmt->expr) {
            if (stmt->expr->kind == EXPR_CALL) result = true;
        }
    }
    return result;
}

static inline Operand op_reg(i32 id, u8 size) {
    Operand result = {
        .kind = OP_REG,
        .size = size,
        .vreg = id,
    };
    return result;
}

static inline Operand op_stack(i64 offset, u8 size) {
    Operand result = {
        .kind = OP_STACK,
        .size = size,
        .stack_slot = offset,
    };
    return result;
}

static inline Operand op_imm(i64 value, u8 size) {
    Operand result = {
        .kind = OP_IMM,
        .size = size,
        .imm = value,
    };
    return result;
}

static inline Operand op_label_addr(void *symbol) {
    Operand result = {
        .kind = OP_LABEL,
        .symbol = symbol,
    };
    return result;
}

static inline Operand make_operand(Expr *expr) {
    switch (expr->kind) {
        case EXPR_LITERAL:
            if (expr->tav.value.kind == VALUE_INT) {
                return op_imm(expr->tav.value.integer_value, expr->tav.type->size);
            }
            if (expr->tav.value.kind == VALUE_STRING) {
                return op_label_addr(&expr->tav.value.string_value);
            }
            debug_assert(0);
        case EXPR_NAME:
            return op_stack(expr->symbol->stack_offset, expr->symbol->type->size);
        default: {
            RegHandle reg = codegen_expr(expr, -1);
            return op_reg(reg, expr->tav.type->size);
        }
    }
}

RegHandle codegen_expr(Expr *expr, i32 force_reg) {
    if (!expr) return -1;

    switch (expr->kind) {
        case EXPR_LITERAL: {
            RegHandle reg = alloc_reg(force_reg);
            TypeAndValue tav = expr->tav;
            switch (tav.value.kind) {
                case VALUE_INT: {
                    i32 size = tav.type->size;
                    emit_instr(INSTR_LOAD, op_imm(tav.value.integer_value, size), op_reg(reg, size));
                    return reg;
                }
                case VALUE_STRING: {
                    String8 *string = table_intern(&GLOBAL_STRING_TABLE, &expr->tav.value.string_value);
                    emit_instr(INSTR_LEA, op_label_addr(string), op_reg(reg, expr->tav.type->size));
                    // fprintf(GLOBAL_ASM_HANDLE, "    leaq str_%p(%%rip), %s\n", (void *)string, reg_name(reg).str) ;
                    return reg;
                }
                case VALUE_INVALID: {
                    debug_assert(0);
                }
            }
            return reg;
        }
        case EXPR_BINARY: {
            Expr *left  = expr->binary.left;
            Expr *right = expr->binary.right;
            RegHandle reg1 = codegen_expr(left, REG_RDX);
            RegHandle reg2 = codegen_expr(right, REG_RAX);
            // TODO: This is cooked, i need to be able to have access to the type on the expr. Or i need to be able to determine when to access the type through the symbol instead.
            if (expr->binary.op == TOKEN_PLUS) emit_instr(INSTR_ADD, op_reg(reg1, left->symbol->type->size), op_reg(reg2, right->tav.type->size));
            free_reg(reg1);
            return reg2;
        }
        //TODO: This can refer to things other than local variables, so we need to determine storage class and emit
        // the instruction based on that. Right now we assume only locals are a thing
        case EXPR_NAME: {
            RegHandle reg = alloc_reg(force_reg);
            Symbol *symbol = expr->symbol;

            emit_instr(INSTR_LOAD, op_stack(symbol->stack_offset, symbol->type->size), op_reg(reg, symbol->type->size));
            return reg;
        }
        case EXPR_COMPOUND: {
            eprintf("EXPR_COMPOUND CODEGEN NOT IMPLEMENTED\n");
            debug_assert(0);
        }
        case EXPR_CALL: {
            ExprList params = expr->call.params;
            
            // TODO: This is disgusting, clean up
            for (u64 i = params.size; i > 0; i--) {
                u64 idx = i - 1;

                Expr *param_expr = params.items[idx];
                if (idx < array_count(param_regs64)) {
                    Regs64 param_reg_handle = param_regs64[idx];
                    Register *param_reg = &regs[param_reg_handle];

                    debug_assert(!param_reg->in_use && "Parameter register allocated while already in use");
                    
                    codegen_expr(param_expr, param_reg_handle);
                }
                else {
                    if (param_expr->tav.mode == ADRESSING_CONSTANT) {
                        fprintf(GLOBAL_ASM_HANDLE, "    pushq $%d\n", param_expr->tav.value.integer_value);
                    } else {
                        Symbol *param = param_expr->symbol;
                        fprintf(GLOBAL_ASM_HANDLE, "    pushq -%ld(%%rbp)\n", param->stack_offset);
                    }
                }
            }

            fprintf(GLOBAL_ASM_HANDLE, "    call %.*s\n", str8_varg(expr->call.expr->name));
            
            free_param_regs();

            return alloc_reg(REG_RAX);
        }
        default:
            debug_assert(0);
    }
}

void codegen_stmt(Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
    case STMT_DECL: {
        Symbol *symbol = stmt->decl->symbol;
        RegHandle reg = codegen_expr(stmt->decl->var.expr, -1);
        emit_instr(INSTR_STORE, op_reg(reg, symbol->type->size), op_stack(symbol->stack_offset, symbol->type->size));
        free_reg(reg);
        break;
    }
    case STMT_EXPR: {
        RegHandle reg = codegen_expr(stmt->expr, -1);
        free_reg(reg);
        break;
    }
    case STMT_RETURN:
        if (stmt->expr) {
            RegHandle reg = codegen_expr(stmt->expr, -1);
            if (reg != REG_RAX) {
                fprintf(GLOBAL_ASM_HANDLE, "    movq %s, %%rax\n", reg_name(reg).str);
            }
            free_reg(reg);
        }
        // fprintf(GLOBAL_ASM_HANDLE, "    popq %%rbp\n");
        fprintf(GLOBAL_ASM_HANDLE, "    leave\n");
        fprintf(GLOBAL_ASM_HANDLE, "    ret\n");
        break;
    
    default:
        debug_assert(0);
        return;
    }
}

void emit_func_prologue(String8 function_name) {
    fprintf(GLOBAL_ASM_HANDLE, "%.*s:\n", str8_varg(function_name));
    fprintf(GLOBAL_ASM_HANDLE, "    pushq %%rbp\n");
    fprintf(GLOBAL_ASM_HANDLE, "    movq %%rsp, %%rbp\n");
}

void codegen_ast(Arena *arena, SymbolList root, String8 output_path) {
    String8 stripped_output_path = str8_strip_ext(output_path);
    String8 asm_output_path = str8_concat(arena, stripped_output_path, str8_lit(".s"));

    FILE *text_tmp = tmpfile();
    if (!text_tmp) {
        eprintf("tmpfile() failed\n");
        exit(EXIT_FAILURE);
    }

    GLOBAL_ASM_HANDLE = text_tmp;

    for (u64 i = 0; i < root.size; i++) {
        Symbol *symbol = root.items[i];
        if (symbol->kind == SYMBOL_FUNC) {
            if (symbol->decl->linkage == LINKAGE_EXTERNAL) {
                continue;
            }

            i64 frame_size = layout_function(symbol);
            emit_func_prologue(symbol->name);
            
            if (function_has_call(symbol)) {
                frame_size = align_pow2(frame_size, 16);
            }

            if (frame_size) {
                fprintf(GLOBAL_ASM_HANDLE, "    subq $%ld, %%rsp\n", frame_size);
            }

            for (u64 j = 0; j < symbol->func.params_count; j++) {
                Symbol *param = symbol->func.params[j];
                Regs64 param_reg_handle = param_regs64[j];
                fprintf(GLOBAL_ASM_HANDLE, "    movq %s, -%ld(%%rbp)\n", regs[param_reg_handle].name.str, param->stack_offset);
            }

            for (u64 j = 0; j < symbol->decl->func.body.size; j++) {
                Stmt *stmt = symbol->decl->func.body.items[j];
                codegen_stmt(stmt);
            }
            
        }
    }

    FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");
    if (!asm_output_handle) {
        fprintf(stderr, "File %.*s doesn't exist\n", str8_varg(asm_output_path));
        exit(EXIT_FAILURE);
    }

    GLOBAL_ASM_HANDLE = asm_output_handle;
    
    // TEMPORARY TESTING
    fprintf(GLOBAL_ASM_HANDLE, ".section .rodata\n"); 
    for (u32 i = 0; i < GLOBAL_STRING_TABLE.capacity; i++) {
        TableEntry entry = GLOBAL_STRING_TABLE.entries[i];
        if (!entry.hash) continue;
        fprintf(GLOBAL_ASM_HANDLE, "str_%p:\n", (void *)entry.key);
        fprintf(GLOBAL_ASM_HANDLE, "    .string \"%.*s\"\n", str8_varg(*(String8 *)entry.key));
    }

    char *asm_init_string = "    .text\n"
                            "    .global _start\n"
                            "_start:\n"
                            "    call main\n"
                            "    movq %rax, %rdi\n"
                            "    movq $60, %rax\n"
                            "    syscall\n";


    fprintf(GLOBAL_ASM_HANDLE, "%s", asm_init_string);
    rewind(text_tmp);
    char buf[4096];
    u64 n;
    while ((n = fread(buf, 1, sizeof(buf), text_tmp)) > 0) {
        fwrite(buf, 1, n, GLOBAL_ASM_HANDLE);
    }

    fclose(text_tmp);
    fclose(asm_output_handle);
}

