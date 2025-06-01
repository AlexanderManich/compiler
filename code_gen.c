#include "parser.h"

void codegen_ast(Arena *arena, ASTNode *root, String8 output_path) {
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

    fprintf(asm_output_handle, "%s", asm_init_string);

    // Main function
    u64 return_value = root->block.items[0]->function_decl.body->block.items[0]->return_val.expr->const_int.value;
    fprintf(asm_output_handle, "main:\n");
    fprintf(asm_output_handle, "    pushq %%rbp\n");
    fprintf(asm_output_handle, "    movq %%rsp, %%rbp\n");
    fprintf(asm_output_handle, "    movq $%ld, %%rax\n", return_value);
    fprintf(asm_output_handle, "    popq %%rbp\n");
    fprintf(asm_output_handle, "    ret\n");
    
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