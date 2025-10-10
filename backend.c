#include "parser.h"

void codegen_ast(Arena *arena, DeclList root, String8 output_path) {
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
        Decl *decl = root.items[i];
        if (decl->kind == DECL_FUNC) {
            ;
            fprintf(asm_output_handle, "%.*s:\n", str8_varg(decl->name));
            fprintf(asm_output_handle, "    pushq %%rbp\n");
            fprintf(asm_output_handle, "    movq  %%rsp, %%rbp\n");
            fprintf(asm_output_handle, "    popq  %%rbp\n");
            
            ParamList params = decl->func.params;
            for (u64 param_idx = 0; param_idx < params.size; param_idx++) {
                FuncParam *param = params.items[param_idx];
                
            }

            fprintf(asm_output_handle, "    ret\n");
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