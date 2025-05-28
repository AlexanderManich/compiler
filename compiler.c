#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "arena.h"
#include "code_gen.h"

u64 calc_cstring_length(char *cstr) {
    u64 i = 0;
    for(; cstr[i]; i += 1);
    return i;
}

String8 str8(u8 *str, u64 size) {
    String8 string = {0};
    string.str  = str;
    string.size = size;
    return string;
}

inline
bool str8_compare(String8 str1, String8 str2) {
    i32 result = 0;

    if (str1.size == str2.size) {
        result = 1;

        for (u64 i = 0; i < str1.size && i < str2.size; i++) {
            bool match = (str1.str[i] == str2.str[i]);

            if (!match) {
                result = 0;
                break;
            }
        }
    }

    return result;
}

String8 read_entire_file(Arena *arena, String8 file_path) {
    String8 result = {0};
    char *file_path_cstr = (char*)file_path.str;
    FILE *file = fopen(file_path_cstr, "rb");

    if (!file) {
        fprintf(stderr, "File doesn't exist %s\n", file_path_cstr);
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

    char *program_cstr = shift(argv, argc);
    printf("Program: %s\n", program_cstr);

    String8 input_file_path = {0};
    while (argc > 0) {
        char *flag_cstr = shift(argv, argc);
        if (str8_compare(str8_cstring(flag_cstr), str8_lit("-o"))) {

            if (argc <= 0) {
                fprintf(stderr, "ERROR: no value provided for flag '%s'\n", flag_cstr);
                exit(EXIT_FAILURE);
            }
            char *output_file_path_cstr = shift(argv, argc);
            printf("Output filepath: %s\n", output_file_path_cstr);
        }
        else {
            input_file_path = str8_cstring(flag_cstr);
        }
    }

    if (!input_file_path.size) {
        fprintf(stderr, "ERROR: No input provided\n");
        return EXIT_FAILURE;
    }

    String8 input = read_entire_file(&global_arena, input_file_path);

    Parser parser = {0};
    parser.arena = global_arena;

    Lexer lexer = {0};
    lexer.input = input;
    lexer.parser = &parser;

    // for (Token *token = peek_next_token(&lexer); token->kind != TOKEN_END; token = peek_next_token(&lexer)) {
    //     printf("%.*s (%s), size: %lu\n", str8_varg(token->text), token_kind_name(*token).str, token->text.size);
    //     eat_token(&lexer);
    // }

    ASTNode *expr = parse_statement(&lexer);
    // print_ast(&global_arena, expr);
    codegen_ast(expr);

    return EXIT_SUCCESS;
}
