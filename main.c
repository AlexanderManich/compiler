#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

static char *shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *result = **argv;
    *argc -= 1;
    *argv += 1;
    return result;
}

char *read_entire_file(char *file_path) {
    FILE *file = fopen(file_path, "r");

    if (file == NULL) {
        fprintf(stderr, "ERROR: failed to open file %s\n", file_path);
        exit(EXIT_FAILURE);
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "ERROR: failed to seek to end of file\n");
    }

    long file_size = ftell(file);

    if (file_size < 0) {
        fprintf(stderr, "ERROR: failed to get file size\n");
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: failed to seek to beginning of file\n");
    }

    char *contents = malloc(file_size + 1);
    
    if (contents == NULL) {
        fprintf(stderr, "Malloc failed\n");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(contents, 1, file_size, file);

    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "ERROR: failed to read file");
        free(contents);
        exit(EXIT_FAILURE);
    }

    contents[bytes_read] = '\0';

    fclose(file);

    return contents;
}

int main(int argc, char **argv) {
    char *program = shift(&argc, &argv);
    printf("Program: %s\n", program);

    char *input_file_path = NULL;

    while (argc > 0) {
        char *flag = shift(&argc, &argv);

        if (strcmp(flag, "-o") == 0) {
            if (argc <= 0) {
                fprintf(stderr, "ERROR: no value provided for flag '%s'\n",
                        flag);
                exit(EXIT_FAILURE);
            }
            char *output_file_path = shift(&argc, &argv);
            printf("Output filepath: %s\n", output_file_path);
        }
        else {
            input_file_path = flag;
        }
    }

    char *input = read_entire_file(input_file_path);

    Lexer lexer = {0};
    lexer.content = input;
    lexer.content_len = strlen(input);

    for (Token token = next_token(&lexer); token.kind != TOKEN_END; token = next_token(&lexer)) {
        printf("%.*s (%s)\n", (int) token.text_len, token.text, token_kind_name(token));
    }

    return EXIT_SUCCESS;
}
