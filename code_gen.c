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

typedef i32 RegHandle;

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
                emit_instr(INSTR_COPY, make_operand(OP_REG, i), make_operand(OP_REG, force_reg));
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

String8 reg_name(i32 reg) {
    return regs[reg].name;
}

static u32 LABEL_COUNT = 0;

u32 label_create() {
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
            debug_assert(src.kind  == OP_STACK);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    movq -%ld(%%rbp), %s\n", src.value, reg_name(dest.value).str);
            break;
        }
        case INSTR_LOAD_IMM: {
            debug_assert(src.kind  == OP_IMM);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    movq $%ld, %s\n", src.value, reg_name(dest.value).str);
            break;
        }
        case INSTR_STORE: {
            debug_assert(src.kind  == OP_REG);
            debug_assert(dest.kind == OP_STACK);
            fprintf(GLOBAL_ASM_HANDLE, "    movq %s, -%ld(%%rbp)\n", reg_name(src.value).str, dest.value);
            break;
        }
        case INSTR_STORE_IMM: {
            debug_assert(src.kind  == OP_IMM);
            debug_assert(dest.kind == OP_STACK);
            fprintf(GLOBAL_ASM_HANDLE, "    movq $%ld, -%ld(%%rbp)\n", src.value, dest.value);
            break;
        }
        case INSTR_COPY: {
            debug_assert(src.kind == OP_REG);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    movq %s, %s\n", reg_name(src.value).str, reg_name(dest.value).str);
            break;
        }
        case INSTR_ADD: {
            debug_assert(src.kind == OP_REG);
            debug_assert(dest.kind == OP_REG);
            fprintf(GLOBAL_ASM_HANDLE, "    addq %s, %s\n", reg_name(src.value).str, reg_name(dest.value).str);
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

Operand make_operand(OperandKind kind, i64 value) {
    Operand result = { 
        .kind = kind, 
        .value = value 
    };
    return result;
}

RegHandle codegen_expr(Expr *expr, i32 force_reg) {
    if (!expr) return -1;

    switch (expr->kind) {
        case EXPR_INT: {
            RegHandle reg = alloc_reg(force_reg);
            emit_instr(INSTR_LOAD_IMM, make_operand(OP_IMM, expr->int_value), make_operand(OP_REG, reg));
            return reg;
        }
        case EXPR_BINARY: {
            RegHandle reg1 = codegen_expr(expr->binary.left, REG_RDX);
            RegHandle reg2 = codegen_expr(expr->binary.right, REG_RAX);
            if (expr->binary.op == TOKEN_PLUS) emit_instr(INSTR_ADD, make_operand(OP_REG, reg1), make_operand(OP_REG, reg2));
            free_reg(reg1);
            return reg2;
        }
        //TODO: This can refer to things other than local variables, so we need to determine storage class and emit
        // the instruction based on that. Right now we assume only locals are a thing
        case EXPR_NAME: {
            RegHandle reg = alloc_reg(force_reg);
            Symbol *symbol = expr->symbol;

            emit_instr(INSTR_LOAD, make_operand(OP_STACK, symbol->stack_offset), make_operand(OP_REG, reg));
            return reg;
        }
        case EXPR_COMPOUND: {
            printf("NOT IMPLEMENTED");
            debug_assert(0);
        }
        case EXPR_CALL: {
            ExprList params = expr->call.params;
            
            for (u64 i = params.size; i > 0; i--) {
                u64 idx = i - 1;
                Symbol *param = params.items[idx]->symbol;
                if (idx < array_count(param_regs64)) {
                    Regs64 param_reg_handle = param_regs64[idx];
                    Register param_reg = regs[param_reg_handle];

                    if (param_reg.in_use) {
                        printf("A PARAMETER REGISTER WAS IN USE WHILE TRYING TO LOAD");
                        debug_assert(0);
                    }

                    fprintf(GLOBAL_ASM_HANDLE, "    movq -%ld(%%rbp), %s\n", param->stack_offset, param_reg.name.str);
                }
                else {
                    fprintf(GLOBAL_ASM_HANDLE, "    pushq -%ld(%%rbp)\n", param->stack_offset);
                }
            }

            fprintf(GLOBAL_ASM_HANDLE, "    call %.*s\n", str8_varg(expr->call.expr->name));

            return REG_RAX;
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
        i64 expr_val = stmt->decl->var.expr->int_value;
        if (stmt->decl->var.expr->kind == EXPR_INT) {
            emit_instr(INSTR_STORE_IMM, make_operand(OP_IMM, expr_val), make_operand(OP_STACK, symbol->stack_offset));
        }
        else {
            RegHandle reg = codegen_expr(stmt->decl->var.expr, -1);
            emit_instr(INSTR_STORE, make_operand(OP_REG, reg), make_operand(OP_STACK, symbol->stack_offset));
            free_reg(reg);
        }
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
    char *asm_init_string = "    .text\n"
                            "    .global _start\n"
                            "_start:\n"
                            "    call main\n"
                            "    movq %rax, %rdi\n"
                            "    movq $60, %rax\n"
                            "    syscall\n";

    String8 asm_output_path = str8_concat(arena, stripped_output_path, str8_lit(".s"));
    FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");
    if (!asm_output_handle) {
        fprintf(stderr, "File %.*s doesn't exist\n", str8_varg(asm_output_path));
        exit(EXIT_FAILURE);
    }
    GLOBAL_ASM_HANDLE = asm_output_handle;

    fprintf(asm_output_handle, "%s", asm_init_string);

    for (u64 i = 0; i < root.size; i++) {
        Symbol *symbol = root.items[i];
        if (symbol->kind == SYMBOL_FUNC) {
            i64 frame_size = layout_function(symbol);
            emit_func_prologue(symbol->name);
            
            if (function_has_call(symbol)) {
                frame_size = align_pow2(frame_size, 16);
            }

            if (frame_size) {
                fprintf(asm_output_handle, "    subq $%ld, %%rsp\n", frame_size);
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

    fclose(asm_output_handle);
}

void asm_compile_and_link(Arena *arena, String8 out_flag) {
    String8 stripped_output_path = str8_strip_ext(out_flag);
    String8 asm_output_path = str8_concat(arena, stripped_output_path, str8_lit(".s"));
    String8 obj_file_path = str8_concat(arena, stripped_output_path, str8_lit(".o"));
    char asm_command[256];
    sprintf(asm_command, "as %.*s -o %.*s", str8_varg(asm_output_path), str8_varg(obj_file_path));

    char link_command[256];
    sprintf(link_command, "ld %.*s -o %s", str8_varg(obj_file_path), out_flag.str);

    system(asm_command);
    system(link_command);
}
