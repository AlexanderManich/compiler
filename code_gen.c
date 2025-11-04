#include "parser.h"
#include "type_check.h"


typedef struct FrameInfo FrameInfo;
struct FrameInfo {
    // TODO: Maybe we should use a HashMap instead for key-value pairs of decl name - frame offset
    String8 locals[32];
    u64     locals_offsets[32];
    u64     locals_size; 
    u64     frame_size;
};

FrameInfo locals_offset(Decl *func_decl) {
    u64 offset = 0;
    FrameInfo frame_info = {0};
    u64 locals_size = 0;
    for (u64 i = 0; i < func_decl->func.body.size; i++) {
        Decl *local = func_decl->func.body.items[i]->decl;
        if (local->kind == DECL_VAR) {
            Symbol *local_symbol = local->symbol;
            offset += local_symbol->type->size;
            frame_info.locals[locals_size] = local->name;
            frame_info.locals_offsets[locals_size] = offset;
            locals_size += 1;
        }
    }
    frame_info.locals_size = locals_size;
    frame_info.frame_size = offset;
    return frame_info;
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

    fprintf(asm_output_handle, "%s", asm_init_string);

    for (u64 i = 0; i < root.size; i++) {
        Symbol *symbol = root.items[i];
        if (symbol->kind == SYMBOL_FUNC) {
            FrameInfo frame_info = locals_offset(symbol->decl);
            // TODO: We need to align up to 16 instead of whatever this is.
            i64 locals_offset = frame_info.frame_size + 16;
            fprintf(asm_output_handle, "%.*s:\n", str8_varg(symbol->name));
            fprintf(asm_output_handle, "    pushq %%rbp\n");
            fprintf(asm_output_handle, "    movq %%rsp, %%rbp\n");

            String8 param_regs32[] = {str8_lit("%edi"), str8_lit("%esi"), str8_lit("%edx"), str8_lit("%ecx"), str8_lit("%r8d"), str8_lit("%r9d")};
            String8 param_regs64[] = {str8_lit("%rdi"), str8_lit("%rsi"), str8_lit("%rdx"), str8_lit("%rcx"), str8_lit("%r8"), str8_lit("%r9")};

            for (u64 param_idx = 0; param_idx < symbol->type->function.param_count; param_idx++) {
                Type *param_type = symbol->type->function.params[i];
                fprintf(asm_output_handle, "    movl %s, -%lu(%%rbp)\n", param_regs32[param_idx].str, locals_offset + param_type->size);
                locals_offset += param_type->size;
            }

            // VERY WORK IN PROGRESS
            for (u64 i = 0; i < symbol->decl->func.body.size; i++) {
                Stmt *stmt = symbol->decl->func.body.items[i];
                if (stmt->kind = STMT_DECL) {
                    if (stmt->decl->var.expr->kind = EXPR_NAME) {

                    }
                }
            }

            fprintf(asm_output_handle, "    popq %%rbp\n");
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
