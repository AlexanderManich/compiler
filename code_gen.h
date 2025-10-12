#pragma once

#include "parser.h"

void codegen_ast(Arena* arena, SymbolList root, String8 output_path);
void asm_compile_and_link(Arena *arena, String8 out_flag);
void codegen_ir(FILE *out);