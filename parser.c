#include "lexer.h"
#include "parser.h"

ASTNode *parse_factor(Lexer *lexer) {
    Token token = next_token(lexer);
    switch (token.kind) {
        case TOKEN_NUMBER:
            ASTNode *node = malloc(sizeof(ASTNode)); 
            node->kind = NODE_INT_LITERAL;
            node->int_literal.value = string_to_int(token.text, token.text_len);
            return node;
        default:
            fprintf(stderr, "Error: Unexpected token '%s'\n");
            exit(EXIT_FAILURE);
    }
}

ASTNode *parse_term(Lexer *lexer) {
    Token token = next_token(lexer);

    ASTNode *node = parse_factor(lexer);

    while (token.kind == TOKEN_PLUS) {
        char op = token.text[0];
        ASTNode *right = parse_factor(lexer); 

        ASTNode *new_node = malloc(sizeof(ASTNode));
        new_node->kind = NODE_BINARY_OP;
        new_node->binary_op.left = node;
        new_node->binary_op.right = right;
        new_node->binary_op.op = op;
        
        node = new_node;
    }

    return node;
}