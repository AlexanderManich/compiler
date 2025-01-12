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

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *contents = malloc(file_size);
    
    if (contents == NULL) {
        fprintf(stderr, "Malloc failed\n");
        exit(EXIT_FAILURE);
    }

    size_t contents_size = fread(contents, file_size, 1, file);

    if (contents_size != 1) {
        fprintf(stderr, "ERROR: failed to read file");
        exit(EXIT_FAILURE);
    }

    contents[file_size] = '\0';

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

    Token token; 
    token = next_token(&lexer);
    while (token.kind != TOKEN_END) {
        printf("%.*s (%s)\n", (int) token.text_len, token.text, token_kind_name(token));
        token = next_token(&lexer);
    }

    return EXIT_SUCCESS;
}
