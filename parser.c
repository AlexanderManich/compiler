#include "lexer.h"
#include "parser.h"
#include "string.h"
#include "compiler.h"

String8 to_operator(Token *token) {
    return token->text;
}

bool is_binary_operator(Token *token) {
    switch (token->kind) {
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_PLUS:
        case TOKEN_MULT:
        case TOKEN_GT:
            return true;
        default:
            return false;
    }
}

i32 get_precedence_from_kind(TokenKind kind) {
    return token_literals[kind].precedence;
}

u64 calc_ast_tree_depth(ASTNode *tree) {
    if (tree == NULL) return 0;

    if (tree->kind == NODE_BINARY_OP) {
        u64 left_depth = calc_ast_tree_depth(tree->binary_op.left);
        u64 right_depth = calc_ast_tree_depth(tree->binary_op.right);
        return 1 + max(left_depth, right_depth);
    }

    if (tree->kind == NODE_RETURN) {
        return 1 + calc_ast_tree_depth(tree->return_val.expr);
    }
    if (tree->kind == NODE_FUNCTION) {
        return 1 + calc_ast_tree_depth(tree->function.body);
    }

    return 1;
}

void print_ast(Arena *arena, ASTNode *tree) {
    CharGrid buffer = {0};
    buffer.height   = calc_ast_tree_depth(tree) * 10;
    buffer.width    = buffer.height * 10;
    buffer.grid     = malloc(buffer.height * buffer.width);
    memset(buffer.grid, ' ', buffer.height * buffer.width);

    _print_ast(arena, buffer, tree, 20, 0);
    for (u32 y = 0; y < buffer.height; ++y) {
        for (u32 x = 0; x < buffer.width; ++x) {
                putchar((char)buffer.grid[y * buffer.width + x]);
        }
        printf("\n");
    }
}

CharGrid _print_ast(Arena *arena, CharGrid buffer, ASTNode *node, u32 x, u32 y) {
    // BUG: When a binary nodes' left and right children are also bin-nodes, the childrens children
    // overwrite each other sometimes. Other things are surely wrong as well.
    if (!node) {
        fprintf(stderr, "Node was 0 while attempting to print ast\n");
        return buffer;
    }
    if (node->kind == NODE_BINARY_OP) {
        memcpy(&buffer.grid[y * buffer.width + x - 1], node->binary_op.op.str, node->binary_op.op.size);
        buffer.grid[(y + 1) * buffer.width + x - 1] = '/';
        buffer.grid[(y + 1) * buffer.width + x + 1] = '\\';
        
        u32 offset = 4;

        _print_ast(arena, buffer, node->binary_op.left, x - offset, y + 2);
        _print_ast(arena, buffer, node->binary_op.right, x + offset, y + 2);
    }
    if (node->kind == NODE_VAR) {
       memcpy(&buffer.grid[y * buffer.width + x], node->variable.name.str, node->variable.name.size); 
    }
    if (node->kind == NODE_INT) {
        String8 i32_as_s8 = str8_from_i32(arena ,node->integer.value);
        memcpy(&buffer.grid[y * buffer.width + x], i32_as_s8.str, i32_as_s8.size);
    }
    if (node->kind == NODE_RETURN) {
        String8 node_name = node_kind_name(node);
        memcpy(&buffer.grid[y * buffer.width + x], node_name.str, node_name.size);
        memcpy(&buffer.grid[(y + 1) * buffer.width + x], "|", 1);
        _print_ast(arena, buffer, node->return_val.expr, x, y + 2);
    }
    if (node->kind == NODE_FUNCTION) {
        // TODO: Change from sprintf, when i have str8 format string
        String8 node_name = node_kind_name(node);
        sprintf((char *)(buffer.grid + y * buffer.width + (x - (node_name.size + node->function.name.size) / 2)),
        "%s (%.*s)", node_name.str, str8_varg(node->function.name));

        buffer.grid[(y + 1) * buffer.width + x] = '|';

        _print_ast(arena, buffer, node->function.body, x, y + 2);
    }
    if (node->kind == NODE_BLOCK) {
        u64 size = node->block.size;
        for (u64 i = 0; i < size; i++) {
            _print_ast(arena, buffer, node->block.statements[i], x + i * 20, y);
        }
    }
    if (node->kind == NODE_VAR_DECL) {
        String8 node_name = node_kind_name(node);
        sprintf((char *)(buffer.grid + y * buffer.width + (x - (node_name.size + node->var_decl.name.size)/ 2)),
        "%s (%.*s)", node_name.str, str8_varg(node->var_decl.name));
        memcpy(&buffer.grid[(y + 1) * buffer.width + x], "|", 1);
        _print_ast(arena, buffer, node->var_decl.expr, x, y + 2);
    }

    return buffer;
}

String8 node_kind_name(ASTNode *node) {
    switch (node->kind) {
        case NODE_VAR:
            return str8_lit("var");
        case NODE_VAR_DECL:
            return str8_lit("var decl");
        case NODE_VAR_ASSIGN:
            return str8_lit("var assign");
        case NODE_INT:
            return str8_lit("integer");
        case NODE_BINARY_OP:
            return str8_lit("binary operator");
        case NODE_RETURN:
            return str8_lit("return");
        case NODE_FUNCTION:
            return str8_lit("function");
        default:
            fprintf(stderr, "No name defined for node: %d", node->kind);
    }

    return (String8){0};
}

// Make nodes
ASTNode *make_number(Arena *arena, Token *token) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_INT;
    node->integer.value = i32_from_str8(token->text);
    return node;
}

ASTNode *make_binary(Arena *arena, ASTNode *left, String8 operator, ASTNode *right) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_BINARY_OP;
    node->binary_op.left = left;
    node->binary_op.op = operator;
    node->binary_op.right = right;
    return node;
}

ASTNode *make_return(Arena *arena, ASTNode *expr) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_RETURN;
    node->return_val.expr = expr;
    return node;
}

ASTNode *make_function(Arena *arena, String8 name, ASTNode *body) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_FUNCTION;
    node->function.name = name;
    node->function.body = body;
    return node;
}

ASTNode *make_if(Arena *arena, ASTNode *cond, ASTNode *if_body, ASTNode *else_body) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_IF;
    node->_if.cond = cond;
    node->_if.if_body = if_body;
    node->_if.else_body = else_body;
    return node;
}

ASTNode *make_variable(Arena *arena, Token *token) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_VAR;
    node->variable.name = token->text;
    return node;
}

ASTNode *make_var_decl(Arena *arena, String8 name, ASTNode *expr) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_VAR_DECL;
    node->var_decl.name = name;
    node->var_decl.expr = expr;
    return node;
}

ASTNode *make_var_assign(Arena *arena, String8 name, ASTNode *expr) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_VAR_ASSIGN;
    node->var_assign.name = name;
    node->var_assign.expr = expr;
    return node;
}

// Parsing

ASTNode *parse_leaf(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    eat_token(lexer);
    ASTNode* result = {0};
    switch (next.kind) {
        case TOKEN_IDENT: {
            result = make_variable(&lexer->parser->arena, &next);
            return result;
        }
        case TOKEN_NUMBER: {
            result = make_number(&lexer->parser->arena, &next);
            return result;
        }
        default:
            fprintf(stderr, "Error: Unexpected token '%.*s'\n", str8_varg(next.text));
            exit(EXIT_FAILURE);
    }
}

ASTNode* parse_increasing_precedence(Lexer *lexer, ASTNode *left, i32 min_precedence) {
    Token next = *peek_next_token(lexer);

    if (!is_binary_operator(&next)) return left;

    i32 next_prec = get_precedence_from_kind(next.kind);

    if (min_precedence <= next_prec) {
        eat_token(lexer);
        ASTNode *right = parse_expression(lexer, next_prec);
        return make_binary(&lexer->parser->arena, left, to_operator(&next), right);
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
    Token next = *peek_next_token(lexer);
    eat_token(lexer);

    ASTNode *statement = {0};
    
    if (next.kind == TOKEN_KEYWORD_IF) {
        statement = parse_if(lexer);
    }     
    if (next.kind == TOKEN_IDENT) {
        String8 name = next.text;
        next = *peek_next_token(lexer);
        if (next.kind == TOKEN_COLON) {
            eat_token(lexer);
            next = *peek_next_token(lexer);
            if (next.kind == TOKEN_COLON) {
                eat_token(lexer);
                statement = parse_var_decl(lexer, name);
                next = *peek_next_token(lexer);
                if (next.kind != TOKEN_SEMICOLON) {
                    print_parse_error(lexer, &next, "Expected ';'");
                }
                eat_token(lexer);
            }
        }
        else if (next.kind == TOKEN_EQUAL) {
            eat_token(lexer);
            statement = parse_var_assign(lexer, name);
        }
    }
    if (next.kind == TOKEN_KEYWORD_RETURN) {
        statement = parse_return(lexer);
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_SEMICOLON) {
            print_parse_error(lexer, &next, "Expected ';'");
        }
        eat_token(lexer);
    }

    next = *peek_next_token(lexer);
    return statement;
}

void block_da_append(Arena *arena, ASTNode *da, ASTNode *statement) {
    if (da->block.size >= da->block.capacity) {
        da->block.capacity = da->block.capacity == 0 ? 256 : da->block.capacity*2;
        da->block.statements = push_array(arena, ASTNode, da->block.capacity * sizeof(*da->block.statements))
    }
    da->block.statements[da->block.size++] = statement;
}

ASTNode *parse_statements(Lexer *lexer) {
    ASTNode *statements = push_array(&lexer->parser->arena, ASTNode, 1);
    statements->kind = NODE_BLOCK;

    for (Token next = *peek_next_token(lexer); next.kind != TOKEN_RIGHT_CURLY && next.kind != TOKEN_END; next = *peek_next_token(lexer)) {       
        ASTNode *statement = parse_statement(lexer);
        block_da_append(&lexer->parser->arena, statements, statement);
    }


    return statements;
}

ASTNode *parse_function(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    eat_token(lexer);
    
    if (next.kind != TOKEN_IDENT) {
        print_parse_error(lexer, &next, "Expected identifier");
    }
    
    String8 name = next.text;

    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_LEFT_PAREN) {
        print_parse_error(lexer, &next, "Expected '('");
    }
    eat_token(lexer);

    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_RIGHT_PAREN) {
        print_parse_error(lexer, &next, "Expected ')'");
    }
    eat_token(lexer);

    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_LEFT_CURLY) {
        print_parse_error(lexer, &next, "Expected '{'");
    }
    eat_token(lexer);
    ASTNode *body = parse_statements(lexer);

    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_RIGHT_CURLY) {
        print_parse_error(lexer, &next, "Expected '}'");
    }
    eat_token(lexer);
 
    ASTNode *function = make_function(&lexer->parser->arena, name, body);

    return function;
}

ASTNode *parse_var_decl(Lexer *lexer, String8 name) {
    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *var_decl = make_var_decl(&lexer->parser->arena, name, expr);
    return var_decl;
}

ASTNode *parse_var_assign(Lexer *lexer, String8 name) {
    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *var_assign = make_var_assign(&lexer->parser->arena, name, expr);
    return var_assign;
}

ASTNode *parse_if(Lexer *lexer) {
    ASTNode *cond = parse_expression(lexer, -1);
    
    Token next = *peek_next_token(lexer);

    if (next.kind != TOKEN_LEFT_CURLY) {
        print_parse_error(lexer, &next, "Expected '{");
    }
    eat_token(lexer);

    ASTNode *if_body = parse_statement(lexer);

    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_RIGHT_CURLY) {
        print_parse_error(lexer, &next, "Expected '}");
    }
    eat_token(lexer);

    ASTNode *else_body = {0};
    if (peek_token(lexer, 1)->kind == TOKEN_KEYWORD_ELSE)  {
        eat_token(lexer);
        else_body = parse_statement(lexer);
    }

    ASTNode *if_statement = make_if(&lexer->parser->arena, cond, if_body, else_body);

    return if_statement;
}

ASTNode *parse_return(Lexer *lexer) {
    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *return_statement = make_return(&lexer->parser->arena, expr);

    return return_statement;
}

void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "l%ld:c%d: ", lexer->current_line, token->end_char - token->start_char);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, " before '%.*s' \n", str8_varg(token->text));
    fprintf(stderr, "%.*s\n", (int)lexer->input_cursor, lexer->input.str + lexer->current_char);
    fprintf(stderr, "%*s^\n", (token->end_char - token->start_char) - 1, "");

    va_end(args);
}

i32 i32_from_str8(String8 str) {
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

String8 str8_from_i32(Arena *arena, i32 num) {
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

    str.str = push_array(arena, u8, length);
    str.size = length;

    while (num > 0) {
        str.str[--length] = (num % 10) + '0';  
        num /= 10;
    }

    return str;
}

