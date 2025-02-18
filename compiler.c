#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "lexer.h"

static String8 shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *string = **argv;
    String8 result = S8CString(string);
    *argc -= 1;
    *argv += 1;
    return result;
}

size_t CalculateCStringLength(char *cstr) {
    size_t i = 0;
    for(; cstr[i]; i += 1);
    return i;
}

String8 S8(char *str, size_t size) {
    String8 string;
    string.str  = str;
    string.size = size;
    return string;
}

String8 read_entire_file(String8 file_path) {
    String8 result = {0};
    FILE *file = fopen(file_path.str, "r");

    if (file) {
        fseek(file, 0, SEEK_END);
        result.size = ftell(file);
        fseek(file, 0, SEEK_SET);
        result.str = malloc(result.size);
        size_t bytes_read = fread(result.str, 1, result.size, file);

        if (bytes_read != (size_t)result.size) {
            fprintf(stderr, "ERROR: failed to read file");
            free(result.str);
            exit(EXIT_FAILURE);
        }

        fclose(file);

        return result;
    }
    fprintf(stderr, "Couldn't find file: %s\n", file_path.str);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    String8 program = shift(&argc, &argv);
    printf("Program: %s\n", program.str);

    String8 input_file_path;

    while (argc > 0) {
        String8 flag = shift(&argc, &argv);

        if (strcmp(flag.str, "-o") == 0) {
            if (argc <= 0) {
                fprintf(stderr, "ERROR: no value provided for flag '%s'\n",
                        flag.str);
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
    lexer.input = input;

    for (Token token = next_token(&lexer); token.kind != TOKEN_END; token = next_token(&lexer)) {
        printf("%.*s (%s)\n", (int) token.text_len, token.text, token_kind_name(token));
    }

    return EXIT_SUCCESS;
}
