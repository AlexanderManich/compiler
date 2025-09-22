#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "type_check.h"
#include "parser.h"
#include "code_gen.h"
#include "ir.h"
#include "base/base_include.h"

String8 read_entire_file(Arena *arena, String8 file_path) {
    String8 result = {0};
    char *file_path_cstr = (char*)file_path.str;
    FILE *file = fopen(file_path_cstr, "rb");

    if (!file) {
        fprintf(stderr, "File doesn't exist %s\n", file_path_cstr);
        exit(EXIT_FAILURE);
    } 

    fseek(file, 0, SEEK_END);
    result.size = ftell(file);
    fseek(file, 0, SEEK_SET);
    result.str = push_array(arena, u8, result.size);
    fread(result.str, result.size, 1, file);
    fclose(file);

    return result;
}

int main(int argc, char **argv) {
    u64 global_arena_size = MB(1);
    u8 global_arena_buffer[global_arena_size];
    Arena global_arena = {0};
    arena_init(&global_arena, global_arena_buffer, global_arena_size);
    
    String8 program = str8_shift_args(&argv, &argc);
    printf("Program: %s\n", program.str);

    String8 input_file_path = {0};
    String8 output_file_path = {0};
    while (argc > 0) {
        String8 flag = str8_shift_args(&argv, &argc);
        if (str8_compare(flag, str8_lit("-o"))) {

            if (argc <= 0) {
                fprintf(stderr, "ERROR: no value provided for flag '%s'\n", flag.str);
                return EXIT_FAILURE;
            }
            output_file_path = str8_shift_args(&argv, &argc),
            printf("Output filepath: %s\n", output_file_path.str);
        }
        else {
            input_file_path = flag;
        }
    }

    if (!input_file_path.size) {
        fprintf(stderr, "ERROR: No input provided\n");
        return EXIT_FAILURE;
    }

    String8 input = read_entire_file(&global_arena, input_file_path);

    u64 parser_arena_size = MB(1);
    u8 parser_arena_buffer[parser_arena_size];
    Arena parser_arena = {0};
    arena_init(&parser_arena, parser_arena_buffer, parser_arena_size);

    TypeSet type_set = {0};

    Parser parser = {0};
    parser.arena = &parser_arena;

    Lexer lexer = {0};
    lexer.input = input;
    lexer.parser = &parser;

    // for (Token *token = peek_next_token(&lexer); token->kind != TOKEN_END; token = peek_next_token(&lexer)) {
    //     printf("%.*s (%s), size: %lu\n", str8_varg(token->text), token_kind_name(*token).str, token->text.size);
    //     eat_token(&lexer);
    // }

    DeclList program_ast = parse_program(&lexer);

    Scope global_scope = {0};
    global_scope.arena = &parser_arena;
    global_scope.types = &type_set;

    // dump_ast(program_ast, 0);
    // collect_declarations(program_ast, &global_scope);

    // if (!output_file_path.str) {
    //     output_file_path = str8_lit("main");
    // }
    // codegen_ast(&global_arena, program_ast, output_file_path);
    // asm_compile_and_link(&global_arena, output_file_path);

    return EXIT_SUCCESS;
}

#include "base/base_include.c"
#include "lexer.c"
#include "parser.c"
#include "type_check.c"
// #include "ir.c"
// #include "code_gen.c"
