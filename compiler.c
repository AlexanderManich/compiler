#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "lexer.h"
#include "parser.h"

static String8 shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *string = **argv;
    String8 result = str8_cstring(string);
    *argc -= 1;
    *argv += 1;
    return result;
}

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

String8 read_entire_file(String8 file_path) {
    String8 result = {0};
    char *file_path_cstr = (char*)file_path.str;
    FILE *file = fopen(file_path_cstr, "rb");

    if (!file) {
        fprintf(stderr, "File doesn't exist %s\n", file_path_cstr);
    } 

    fseek(file, 0, SEEK_END);
    result.size = ftell(file);
    fseek(file, 0, SEEK_SET);
    result.str = malloc(result.size);
    fread(result.str, result.size, 1, file);
    fclose(file);

    return result;
}

int main(int argc, char **argv) {
    String8 program = shift(&argc, &argv);
    printf("Program: %s\n", program.str);

    String8 input_file_path;

    while (argc > 0) {
        String8 flag = shift(&argc, &argv);
        if (str8_compare(flag, str8_lit("-o"))) {

            if (argc <= 0) {
                fprintf(stderr, "ERROR: no value provided for flag '%s'\n", flag.str);
                exit(EXIT_FAILURE);
            }
            String8 output_file_path = shift(&argc, &argv);
            printf("Output filepath: %s\n", output_file_path.str);
        }
        else {
            input_file_path = flag;
        }
    }

    String8 input = read_entire_file(input_file_path);

    Lexer lexer = {0};
    lexer.current_line = 1;
    lexer.input = input;

    Token *token = peek_token(&lexer, 0);
    while (token->kind != TOKEN_END) {
        printf("%.*s (%s)\n", (int) token->text.size, token->text.str, token_kind_name(*token).str);
        eat_token(&lexer);
        token = peek_token(&lexer, 0);
    }

    // for (Token *token = peek_token(&lexer, 0); token->kind != TOKEN_END; token = peek_token(&lexer, 0)) {
    //     printf("%.*s (%s)\n", (int) token->text.size, token->text.str, token_kind_name(*token).str);
    //     eat_token(&lexer);
    // }

    // ASTNode *expr = parse_function(&lexer);
    // print_ast(expr);

    return EXIT_SUCCESS;
}
