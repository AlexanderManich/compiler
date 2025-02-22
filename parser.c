#include "lexer.h"
#include "parser.h"
#include "string.h"

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
        case TOKEN_GT:
            return true;
        default:
            return false;
    }
}

i32 get_precedence(Token token) {
    switch (token.kind) {
        case TOKEN_GT:
            return 1; 
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 2;
        case TOKEN_MULT:
            return 3;
        default:
            return -1;
    }
}

u64 calc_ast_tree_depth(ASTNode *tree) {
    if (tree == NULL) return 0;

    if (tree->kind == NODE_BINARY_OP) {
        u64 left_depth = calc_ast_tree_depth(tree->binary_op.left);
        u64 right_depth = calc_ast_tree_depth(tree->binary_op.right);
        return 1 + (left_depth > right_depth ? left_depth : right_depth);
    }

    return 1;
}

void print_ast(ASTNode *tree) {
    CharGrid buffer = {0};
    buffer.height   = calc_ast_tree_depth(tree) * 2;
    buffer.width    = buffer.height * 10;
    buffer.grid     = malloc(buffer.height * buffer.width);
    memset(buffer.grid, ' ', buffer.height * buffer.width);

    _print_ast(buffer, tree, 40, 0);
    for (u32 y = 0; y < buffer.height; ++y) {
        for (u32 x = 0; x < buffer.width; ++x) {
                putchar((char)buffer.grid[y * buffer.width + x]);
        }
        printf("\n");
    }
    free(buffer.grid);
}

CharGrid _print_ast(CharGrid buffer, ASTNode *node, u32 x, u32 y) {
    // BUG: When a binary nodes' left and right children are also bin-nodes, the childrens children
    // overwrite each other sometimes. Other things are surely wrong as well.
    if (node->kind == NODE_BINARY_OP) {
        sprintf(((char *)&buffer.grid[y * buffer.width + x]), "%.*s", (int)node->binary_op.op.size, node->binary_op.op.str);
        buffer.grid[(y + 1) * buffer.width + x - 1] = '/';
        buffer.grid[(y + 1) * buffer.width + x + 1] = '\\';
        
        u32 offset = 4;

        _print_ast(buffer, node->binary_op.left, x - offset, y + 2);
        _print_ast(buffer, node->binary_op.right, x + offset, y + 2);
    }
    if (node->kind == NODE_INT_LITERAL) {
        sprintf((char *)(buffer.grid + y * buffer.width + x), "%d", node->int_literal.value);
    }

    return buffer;
}

ASTNode *parse_leaf(Lexer *lexer) {
    Token next = next_token(lexer);
    switch (next.kind) {
        case TOKEN_NUMBER: {
            return make_number(next);
        }
        default:
            fprintf(stderr, "Error: Unexpected token '%.*s'\n", (int)next.text.size, next.text.str);
            exit(EXIT_FAILURE);
    }
}

ASTNode* parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence) {
    Token next = peek_token(lexer);

    if (!is_binary_operator(next)) return left;

    i32 next_prec = get_precedence(next);

    if (min_precedence <= next_prec) {
        next_token(lexer);
        ASTNode *right = parse_expression(lexer, next_prec);
        return make_binary(left, to_operator(next), right);
    }
    else {
        return left;
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

