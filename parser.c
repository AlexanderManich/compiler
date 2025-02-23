#include "lexer.h"
#include "parser.h"
#include "string.h"

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

    if (tree->kind == NODE_RETURN) {
        return 1 + calc_ast_tree_depth(tree->return_val.expr);
    }
    if (tree->kind == NODE_FUNCTION) {
        return 1 + calc_ast_tree_depth(tree->function.body);
    }

    return 1;
}

void print_ast(ASTNode *tree) {
    CharGrid buffer = {0};
    buffer.height   = calc_ast_tree_depth(tree) * 2;
    buffer.width    = buffer.height * 10;
    buffer.grid     = malloc(buffer.height * buffer.width);
    memset(buffer.grid, ' ', buffer.height * buffer.width);

    _print_ast(buffer, tree, 20, 0);
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
        // sprintf((char *)&buffer.grid[y * buffer.width + x], "%.*s", (int)node->binary_op.op.size, node->binary_op.op.str);
        memcpy(&buffer.grid[y * buffer.width + x], node->binary_op.op.str, node->binary_op.op.size);
        buffer.grid[(y + 1) * buffer.width + x - 1] = '/';
        buffer.grid[(y + 1) * buffer.width + x + 1] = '\\';
        
        u32 offset = 4;

        _print_ast(buffer, node->binary_op.left, x - offset, y + 2);
        _print_ast(buffer, node->binary_op.right, x + offset, y + 2);
    }
    if (node->kind == NODE_INT_LITERAL) {
        String8 i32_as_s8 = i32_to_S8(node->int_literal.value);
        memcpy(&buffer.grid[y * buffer.width + x], i32_as_s8.str, i32_as_s8.size);
    }
    if (node->kind == NODE_RETURN) {
        memcpy(&buffer.grid[y * buffer.width + x], "Return", 6);
        memcpy(&buffer.grid[(y + 1) * buffer.width + x], "|", 1);
        _print_ast(buffer, node->return_val.expr, x, y + 2);
    }
    if (node->kind == NODE_FUNCTION) {
        // TODO: Change from sprintf, when i have S8FormatString
        sprintf((char *)(buffer.grid + y * buffer.width + x), "Function (%.*s)", (int)node->function.name.size, node->function.name.str);
        buffer.grid[(y + 1) * buffer.width + x] = '|';
        _print_ast(buffer, node->function.body, x, y + 2);
    }

    return buffer;
}

String8 node_kind_name(ASTNode node) {
    switch (node.kind) {
        case NODE_VAR:
            return S8Lit("Variable");
        case NODE_INT_LITERAL:
            return S8Lit("Integer literal");
        case NODE_BINARY_OP:
            return S8Lit("Binary operator");
        case NODE_RETURN:
            return S8Lit("Return");
        case NODE_FUNCTION:
            return S8Lit("Function");
        default:
            fprintf(stderr, "No name defined for node");
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

ASTNode *make_return(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->kind = NODE_RETURN;
    node->return_val.expr = expr;
    return node;
}

ASTNode *make_function(String8 name, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->kind = NODE_FUNCTION;
    node->function.name = name;
    node->function.body = body;
    return node;
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

ASTNode *parse_statement(Lexer *lexer) {
    Token next = next_token(lexer);

    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *statement = make_return(expr);

    next = next_token(lexer);
    if (next.kind != TOKEN_SEMICOLON) fprintf(stderr, "Expected ;\n");

    return statement;
}

ASTNode *parse_function(Lexer *lexer) {
    Token next = next_token(lexer);
    if (next.kind != TOKEN_SYMBOL) fprintf(stderr, "Expected identifier (function name)\n");
    String8 name = next.text;

    next = next_token(lexer);
    if (next.kind != TOKEN_LEFT_PAREN)  fprintf(stderr, "Expected (\n");
    next = next_token(lexer);
    if (next.kind != TOKEN_RIGHT_PAREN) fprintf(stderr, "Expected )\n");

    next = next_token(lexer);
    if (next.kind != TOKEN_LEFT_CURLY)  fprintf(stderr, "Expected {\n");

    ASTNode *body = parse_statement(lexer);

    next = next_token(lexer);
    if (next.kind != TOKEN_RIGHT_CURLY) fprintf(stderr, "Expected }\n");

    ASTNode *function = make_function(name, body);

    return function;
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

String8 i32_to_S8(i32 num) {
    String8 str = {0};

    if (num == 0) {
        return str;
    }

    u32 temp = num;
    u32 length = 0;

    while (temp > 0) {
        temp /= 10;
        length++;
    }

    str.str = malloc(length * sizeof(u8));
    str.size = length;

    while (num > 0) {
        str.str[--length] = (num % 10) + '0';  
        num /= 10;
    }

    return str;
}

