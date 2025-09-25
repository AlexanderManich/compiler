#include "parser.h"
#include "ir.h"

// void codegen_ast(Arena *arena, ASTNode *root, String8 output_path) {
//     String8 stripped_output_path = str8_strip_ext(output_path);
//     char *asm_init_string = "    .text\n"
//                             "    .global _start\n"
//                             "_start:\n"
//                             "    call main\n"
//                             "    movq %rax, %rdi\n"
//                             "    movq $60, %rax\n"
//                             "    syscall\n";

//     String8 asm_output_path = str8_concat(arena, stripped_output_path, str8_lit(".s"));
//     FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");

//     fprintf(asm_output_handle, "%s", asm_init_string);

//     for (u64 i = 0; i < root->block.size; i++) {
//         ASTNode *node = root->block.items[i];
//         if (node->kind == NODE_FUNCTION_DECL) {
//             fprintf(asm_output_handle, "%.*s:\n", str8_varg(node->function_decl.name));
//             fprintf(asm_output_handle, "    pushq %%rbp\n    movq %%rsp, %%rbp\n");
//             ir_gen_function(node);
//             codegen_ir(asm_output_handle); 
//         }
//     }

//     fclose(asm_output_handle);
// }

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

void codegen_ir(FILE *out) {
    // TODO: Map virtual registers to real registers
    const char *reg_names[] = {"%rax", "%rbx", "%rcx", "%rdx"};
    for (int i = 0; i < ir_count; ++i) {
        Instruction *inst = &ir_code[i];
        switch (inst->op) {
            case IR_LOAD_CONST:
                fprintf(out, "    movq $%d, %s\n", inst->imm, reg_names[inst->dest]);
                break;
            case IR_MOV:
                fprintf(out, "    movq %s, %s\n", reg_names[inst->src1], reg_names[inst->dest]);
                break;
            case IR_RET:
                fprintf(out, "    movq %s, %%rax\n", reg_names[inst->src1]);
                fprintf(out, "    popq %%rbp\n");
                fprintf(out, "    ret\n");
                break;
        }
    }
}