void x86_init_generator(x86Generator *generator, FILE *output_handle) {
    generator->output_handle = output_handle;
}

static inline void emit_line(FILE *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(handle, format, args);
    fprintf(handle, "\n");
    va_end(args);
}

void emit_func_prologue(x86Generator *generator, String8 function_name) {
    // TODO: Temporary 
    // emit_line(generator->output_handle, "    .globl %.*s", str8_varg(function_name));
    emit_line(generator->output_handle, "%.*s:", str8_varg(function_name));
    emit_line(generator->output_handle, "    pushq %%rbp");
    emit_line(generator->output_handle, "    movq %%rsp, %%rbp");
}

void x86_emit_function(x86Generator *generator, IRFunction *function) {
    emit_func_prologue(generator, function->name);
    
    for (u64 block_idx = 0; block_idx < function->blocks.size; block_idx++) {
        IRBlock *block = &function->blocks.items[block_idx];

        for (u64 instr_idx = 0; instr_idx < block->instructions.size; instr_idx++) {
            IRInstr *instr = &block->instructions.items[instr_idx];
            switch (instr->kind) {
                case IR_NOOP:
                    emit_line(generator->output_handle, "");
                    break;
                case IR_MOV: {
                    IROperand dst = instr->operands[0];
                    IROperand src = instr->operands[1];

                    break;
                }
                default:
                    debug_assert(0);
                    break;
            }
        }
    }
}

void x86_emit_program(Arena *arena, IRModule *module, String8 asm_output_path) {
    FILE *asm_output_handle = fopen((char *)asm_output_path.str, "w");
    if (!asm_output_handle) {
        fprintf(stderr, "File %.*s doesn't exist\n", str8_varg(asm_output_path));
        exit(EXIT_FAILURE);
    }

    x86Generator x86_gen = {0};
    x86_init_generator(&x86_gen, asm_output_handle);

    emit_line(x86_gen.output_handle, "    .text");
    emit_line(x86_gen.output_handle, "    .p2align 4");
    emit_line(x86_gen.output_handle, "    .global _start");
    emit_line(x86_gen.output_handle, "_start:");
    emit_line(x86_gen.output_handle, "    subq $8, %%rsp");
    emit_line(x86_gen.output_handle, "    call main");
    emit_line(x86_gen.output_handle, "    addq $8, %%rsp");
    emit_line(x86_gen.output_handle, "    movq %%rax, %%rdi");
    emit_line(x86_gen.output_handle, "    movq $60, %%rax");
    emit_line(x86_gen.output_handle, "    syscall");
    emit_line(x86_gen.output_handle, "");

    for (u32 i = 0; i < module->functions.size; i++) {
        reg_alloc_function(module->functions.items[i]);
        // x86_emit_function(&x86_gen, module->functions.items[i]);
        // emit_line(x86_gen.output_handle, "");
    }

    fclose(asm_output_handle);
}
