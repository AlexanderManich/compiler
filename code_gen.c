#include "parser.h"
#include "type_check.h"

FILE *GLOBAL_ASM_HANDLE = 0;

i64 layout_function(Symbol *func) {
    debug_assert(func->kind == SYMBOL_FUNC);

    i64 offset = 0;

    for (u64 i = 0; i < func->type->function.param_count; i++) {
        Symbol *param = func->type->function.params[i]->symbol;
        offset += param->type->size;
        param->stack_offset = offset;
    }

    for (u64 i = 0; i < func->decl->func.body.size; i++) {
        Stmt *stmt = func->decl->func.body.items[i];
        if (stmt->kind == STMT_DECL) {
            Symbol *local = stmt->decl->symbol;
            offset += local->type->size;
            local->stack_offset = offset;
        }
    }

    return align_pow2(offset, 16);
}

typedef struct Register {
    String8 name;
    b32   in_use;
} Register;

String8 param_regs32[] = {str8_lit_comp("%edi"), str8_lit_comp("%esi"), str8_lit_comp("%edx"), str8_lit_comp("%ecx"), str8_lit_comp("%r8d"), str8_lit_comp("%r9d")};
String8 param_regs64[] = {str8_lit_comp("%rdi"), str8_lit_comp("%rsi"), str8_lit_comp("%rdx"), str8_lit_comp("%rcx"), str8_lit_comp("%r8"), str8_lit_comp("%r9")};

typedef i32 RegHandle;

#define MAX_SCRATCH_REGISTERS 7
static Register regs[7] = {
    { .name = str8_lit_comp("%rbx"), .in_use = 0 },
    { .name = str8_lit_comp("%r10"), .in_use = 0 },
    { .name = str8_lit_comp("%r11"), .in_use = 0 },
    { .name = str8_lit_comp("%r12"), .in_use = 0 },
    { .name = str8_lit_comp("%r13"), .in_use = 0 },
    { .name = str8_lit_comp("%r14"), .in_use = 0 },
    { .name = str8_lit_comp("%r15"), .in_use = 0 },
};

i32 alloc_reg() {
    for (u32 i = 0; i < MAX_SCRATCH_REGISTERS; i++) {
        if (regs[i].in_use == false) {
            regs[i].in_use = true;
            return i;
        };
    }
    return -1;
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

RegHandle codegen_expr(Expr *expr) {
    if (!expr) return -1;

    switch (expr->kind) {
        case EXPR_INT: {
            RegHandle reg = alloc_reg();
            fprintf(GLOBAL_ASM_HANDLE, "    movq $%ld, %s\n", expr->int_value, reg_name(reg).str);
            return reg;
        }
        case EXPR_BINARY: {
        }
        //TODO: This can refer to things other than local variables, so we need to determine storage class and emit
        // the instruction based on that. Right now we assume only locals are a thing
        case EXPR_NAME: {
            RegHandle reg = alloc_reg();
            Symbol *symbol = expr->symbol;
            fprintf(GLOBAL_ASM_HANDLE, "    movq -%ld(%%rbp), %s\n", symbol->stack_offset, reg_name(reg).str);
            return reg;
        }
        default:
            return -1;
    }
}

void codegen_stmt(Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
    case STMT_DECL: {
        RegHandle reg = codegen_expr(stmt->decl->var.expr);
        Symbol *symbol = stmt->decl->symbol;
        fprintf(GLOBAL_ASM_HANDLE, "    movq %s, -%ld(%%rbp)\n", reg_name(reg).str, symbol->stack_offset);
        free_reg(reg);
        break;
    }
    case STMT_RETURN:
        if (stmt->expr) {
            RegHandle reg = codegen_expr(stmt->expr);
            fprintf(GLOBAL_ASM_HANDLE, "    movq %s, %%rax\n", reg_name(reg).str);
            free_reg(reg);
        }
        fprintf(GLOBAL_ASM_HANDLE, "    popq %%rbp\n");
        fprintf(GLOBAL_ASM_HANDLE, "    ret\n");
        break;
    
    default:
        debug_assert(0);
        return;
    }
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
            fprintf(asm_output_handle, "%.*s:\n", str8_varg(symbol->name));
            fprintf(asm_output_handle, "    pushq %%rbp\n");
            fprintf(asm_output_handle, "    movq %%rsp, %%rbp\n");
            fprintf(asm_output_handle, "    subq $%ld, %%rbp\n", frame_size);

            for (u64 i = 0; i < symbol->decl->func.body.size; i++) {
                Stmt *stmt = symbol->decl->func.body.items[i];
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
