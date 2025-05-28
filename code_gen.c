#include "parser.h"

void codegen_ast(ASTNode *root) {
    String8 asm_output_path = str8_lit("build/asm.s");
    FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");

    char *asm_init_string = "    .text\n"
                            "    .global _start\n"
                            "_start:\n";

    fprintf(asm_output_handle, "%s", asm_init_string);
    fprintf(asm_output_handle, "    movq $60, %%rax\n");
    fprintf(asm_output_handle, "    movq $%ld, %%rdi\n", root->return_val.expr->integer.value);
    fprintf(asm_output_handle, "    syscall\n");
    
    fclose(asm_output_handle);
    char *asm_command = "as build/asm.s -o build/asm.o";
    char *link_command = "ld build/asm.o -o build/main";

    system(asm_command);
    system(link_command);
}