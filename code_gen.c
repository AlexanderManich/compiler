#include "parser.h"

void codegen_ast(Arena *arena, ASTNode *root, String8 output_path) {
    if (!output_path.str) {
        output_path = str8_lit("main");
    }
    String8 stripped_output_path = str8_strip_ext(output_path);
    char *asm_init_string = "    .text\n"
                            "    .global _start\n"
                            "_start:\n";

    String8 asm_output_path = str8_concat(arena, stripped_output_path, str8_lit(".s"));
    FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");

    fprintf(asm_output_handle, "%s", asm_init_string);
    fprintf(asm_output_handle, "    movq $60, %%rax\n");
    fprintf(asm_output_handle, "    movq $%ld, %%rdi\n", root->return_val.expr->integer.value);
    fprintf(asm_output_handle, "    syscall\n");
    
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