typedef struct x86Generator {
    FILE *output_handle;
} x86Generator;

void x86_init_generator(x86Generator *generator, FILE *output_handle);

void x86_emit_function(x86Generator *generator, IRFunction *function);
void x86_emit_program(Arena *arena, IRModule *module, String8 asm_output_path);

void emit_func_prologue(x86Generator *generator, String8 function_name);