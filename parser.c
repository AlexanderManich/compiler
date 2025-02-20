#include "lexer.h"
#include "parser.h"

String8 node_kind_name(ASTNode node) {
    switch (node.kind) {
        case NODE_VAR:
            return S8Lit("Variable");
        case NODE_INT_LITERAL:
            return S8Lit("Integer literal");
        case NODE_BINARY_OP:
            return S8Lit("Binary operator");
    }

    return S8Lit("");
}

ASTNode *make_number(Token token) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->kind = NODE_INT_LITERAL;
    node->int_literal.value = S8_to_i32(token.text);
    return node;
}

ASTNode *make_binary(ASTNode *left, String8 operator, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->kind = NODE_BINARY_OP;
    node->binary_op.left = left;
    node->binary_op.op = operator;
    node->binary_op.right = right;
    return node;
}

String8 to_operator(Token token) {
    return token.text;
}

bool is_binary_operator(Token token) {
    switch (token.kind) {
        case TOKEN_PLUS:
        case TOKEN_MULT:
            return true;
        default:
            return false;
    }
}

i32 get_precedence(Token token) {
    switch (token.kind) {
        case TOKEN_PLUS:
            return 1;
        case TOKEN_MULT:
            return 2;
        default:
            return -1;
    }
}

ASTNode *parse_leaf(Lexer *lexer) {
    Token token = next_token(lexer);
    switch (token.kind) {
        case TOKEN_NUMBER: {
            return make_number(token);
        }
        default:
            fprintf(stderr, "Error: Unexpected token '%.*s'\n", (int)token.text.size, token.text.str);
            exit(EXIT_FAILURE);
    }
}

ASTNode* parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence) {
    // TODO: This is bugged
    Token next = next_token(lexer);

    if (!is_binary_operator(next)) return left;

    i32 next_prec = get_precedence(next);

    if (next_prec <= min_precedence ) {
        return left;
    }
    else {
        ASTNode *right = parse_expression(lexer, next_prec);
        return make_binary(left, to_operator(next), right);
    }
}

ASTNode *parse_expression(Lexer *lexer, i32 min_precedence) {
    ASTNode *left = parse_leaf(lexer);
    
    while (true) {
        ASTNode *node = parse_increasing_precedence(lexer, left, min_precedence);
        if (node == left) break;
        left = node;
    }

    return left;
}

i32 S8_to_i32(String8 str) {
    i32 result = 0;
    u64 i = 0;

    while (str.str[i] >= '0' && str.str[i] <= '9') {
        result = result * 10 + (str.str[i] - '0');
        i++;
    }

    if (i < str.size) {
        fprintf(stderr, "Error: Invalid integer format: %.*s\n", (i32)str.size, str.str);
        return 0;
    }

    return result;
}

