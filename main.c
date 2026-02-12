#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "type_check.h"
#include "parser.h"
#include "code_gen.h"
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

#define GLOBAL_ARENA_SIZE MB(1)
static u8 global_arena_buffer[GLOBAL_ARENA_SIZE];
#define PARSER_ARENA_SIZE MB(1)
static u8 parser_arena_buffer[PARSER_ARENA_SIZE];
#define TYPE_ARENA_SIZE MB(1)
static u8 type_arena_buffer[TYPE_ARENA_SIZE];
static Arena GLOBAL_TYPE_ARENA = {0};

static SymbolList GLOBAL_ORDERED_SYMBOLS;

#define STRING_TABLE_ARENA_SIZE MB(1)
static u8 string_table_arena_buffer[TYPE_ARENA_SIZE];
static Arena GLOBAL_STRING_TABLE_ARENA;

#define STRING_TABLE_CAPACITY 1024
static Table GLOBAL_STRING_TABLE;

int main(int argc, char **argv) {
    Arena global_arena = {0};
    arena_init(&global_arena, global_arena_buffer, GLOBAL_ARENA_SIZE);
    
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
        fprintf(stderr, "ERROR: No source file provided\n");
        return EXIT_FAILURE;
    }

    String8 input = read_entire_file(&global_arena, input_file_path);

    Arena parser_arena = {0};
    arena_init(&parser_arena, parser_arena_buffer, PARSER_ARENA_SIZE);
    arena_init(&GLOBAL_TYPE_ARENA, type_arena_buffer, TYPE_ARENA_SIZE);

    arena_init(&GLOBAL_STRING_TABLE_ARENA, string_table_arena_buffer, STRING_TABLE_ARENA_SIZE);
    table_init(&GLOBAL_STRING_TABLE, &GLOBAL_STRING_TABLE_ARENA, STRING_TABLE_CAPACITY, table_str8_hash, table_str8_compare, table_str8_copy);
    
    Parser parser = {0};
    parser.arena = &parser_arena;
    parser.file = input_file_path;
    
    Lexer lexer = {0};
    lexer.current_char = 1;
    lexer.current_line = 1;
    lexer.input = input;
    lexer.parser = &parser;
    
    // for (Token *token = peek_next_token(&lexer); token->kind != TOKEN_END; token = peek_next_token(&lexer)) {
    //     printf("%.*s (%s), size: %lu\n", str8_varg(token->text), token_kind_name(token->kind).str, token->text.size);
    //     eat_token(&lexer);
    // }
    
    DeclList program_ast = parse_program(&lexer);
    
    Scope global_scope = {0};

    global_scope.arena = &parser_arena;
    // ast_print(program_ast, 0);
    register_builtin_symbols(&global_scope);
    register_decls(program_ast, &global_scope);
    // symbol_print_decls(global_scope.symbols);
    resolve_symbols(global_scope.symbols, &global_scope);

    for (u64 i = 0; i < GLOBAL_ORDERED_SYMBOLS.size; i++) {
        printf("name: %.*s\n", str8_varg(GLOBAL_ORDERED_SYMBOLS.items[i]->name));
    }

    if (!output_file_path.str) {
        output_file_path = str8_lit("build/main");
    }
    codegen_ast(&global_arena, GLOBAL_ORDERED_SYMBOLS, output_file_path);
    asm_compile_and_link(&global_arena, output_file_path);

    return EXIT_SUCCESS;
}

#include "base/base_include.c"
#include "lexer.c"
#include "parser.c"
#include "type_check.c"
#include "code_gen.c"
