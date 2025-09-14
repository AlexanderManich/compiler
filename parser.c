#include "lexer.h"
#include "parser.h"
#include "base/base_include.h"

String8 to_operator(Token *token) {
    return token->text;
}

b32 is_binary_operator(Token *token) {
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

void dump_ast(ASTNode *node, i32 depth) {
    if (!node) return;
    printf("%*s", depth, "");

    switch (node->kind) {
        case NODE_FUNCTION_DECL:
            printf("function declaration (%.*s)\n", str8_varg(node->function_decl.name));
            if (node->function_decl.parameters) {
                for (u32 i = 0; i < node->function_decl.parameters->parameter_list.size; i++) {
                    dump_ast(node->function_decl.parameters->parameter_list.items[i], depth + 1);
                }
            }
            printf("%*s", depth + 1, "");
            printf("return type (%.*s)\n", str8_varg(node->function_decl.return_type_name));
            dump_ast(node->function_decl.body, depth + 1);
            break;
        case NODE_FUNCTION_CALL:
            printf("function call (%.*s)\n", str8_varg(node->function_call.name));
            if (node->function_call.parameters) {
                for (u32 i = 0; i < node->function_call.parameters->parameter_list.size; i++) {
                    dump_ast(node->function_decl.parameters->parameter_list.items[i], depth + 1);
                }
            }
            break;
        case NODE_DECL_PARAMETER:
            printf("parameter (%.*s: %.*s)\n", str8_varg(node->parameter.name), str8_varg(node->parameter.type_name));
            break;
        case NODE_VAR_DECL:
            printf("variable declaration (%.*s: %.*s)\n", str8_varg(node->var_decl.name), str8_varg(node->var_decl.type_name));
            if (node->var_decl.expr) dump_ast(node->var_decl.expr, depth + 1);
            break;
        case NODE_CONST_INT:
            printf("const integer (%ld)\n", node->const_int.value);
            break;
        case NODE_BINARY_OP:
            printf("binary operator (%.*s)\n", str8_varg(node->binary_op.op));
            dump_ast(node->binary_op.left, depth + 1);
            dump_ast(node->binary_op.right, depth + 1);
            break;
        case NODE_RETURN:
            printf("return\n");
            dump_ast(node->return_val.expr, depth + 1);
            break;
        case NODE_IDENT:
            printf("identifier (%.*s)\n", str8_varg(node->indentifier.name));
            break;
        case NODE_BLOCK:
            printf("block\n");
            for (u32 i = 0; i < node->block.size; i++) {
                dump_ast(node->block.items[i], depth + 1);
            }
            break;
        default:
            printf("%*s", depth, "");
            printf("unknown node kind (%d)\n", node->kind);
    }
}

String8 node_kind_name(ASTNode *node) {
    switch (node->kind) {
        case NODE_IDENT:
            return str8_lit("identifier");
        case NODE_VAR_DECL:
            return str8_lit("variable declaration");
        case NODE_VAR_ASSIGN:
            return str8_lit("variable assign");
        case NODE_CONST_INT:
            return str8_lit("const integer");
        case NODE_BINARY_OP:
            return str8_lit("binary operator");
        case NODE_RETURN:
            return str8_lit("return");
        case NODE_FUNCTION_DECL:
            return str8_lit("function declaration");
        case NODE_FUNCTION_CALL:
            return str8_lit("function call");
        case NODE_BLOCK:
            return str8_lit("block");
        default:
            fprintf(stderr, "No name defined for node: %d\n", node->kind);
    }

    return (String8){0};
}

// Make nodes
ASTNode *make_number(Arena *arena, Token *token) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_CONST_INT;
    node->const_int.value = i32_from_str8(token->text);
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

ASTNode *make_decl_parameter(Arena *arena, String8 name, String8 type_name) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_DECL_PARAMETER;
    node->parameter.name = name;
    node->parameter.type_name = type_name;
    return node;
}

ASTNode *make_function_decl(Arena *arena, String8 name, ASTNode* parameters, ASTNode *body, String8 return_type_name) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_FUNCTION_DECL;
    node->function_decl.name = name;
    node->function_decl.parameters = parameters;
    node->function_decl.body = body;
    node->function_decl.return_type_name = return_type_name;
    return node;
}

ASTNode *make_function_call(Arena *arena, String8 name, ASTNode* parameters) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_FUNCTION_CALL;
    node->function_call.name = name;
    node->function_call.parameters = parameters;
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

ASTNode *make_indentifier(Arena *arena, Token *token) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_IDENT;
    node->indentifier.name = token->text;
    return node;
}

ASTNode *make_var_decl(Arena *arena, String8 name, String8 type_name, ASTNode *expr) {
    ASTNode *node = push_array(arena, ASTNode, 1);
    node->kind = NODE_VAR_DECL;
    node->var_decl.name = name;
    node->var_decl.type_name = type_name;
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
            String8 identifier = next.text;
            if (peek_next_token(lexer)->kind == TOKEN_LEFT_PAREN) {
                eat_token(lexer);
                result = parse_function_call(lexer, identifier);
            } else {
                result = make_indentifier(lexer->parser->arena, &next);
            }
            return result;
        }
        case TOKEN_NUMBER: {
            result = make_number(lexer->parser->arena, &next);
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
        return make_binary(lexer->parser->arena, left, to_operator(&next), right);
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

        Token next = *peek_next_token(lexer);
        if (next.kind == TOKEN_COLON || next.kind == TOKEN_COLON_COLON) {
            statement = parse_var_decl(lexer, name);
        }
    }
    if (next.kind == TOKEN_KEYWORD_RETURN) {
        statement = parse_return(lexer);
    }

    return statement;
}

ASTNode *parse_statements(Lexer *lexer) {
    ASTNode *statements = push_array(lexer->parser->arena, ASTNode, 1);
    statements->kind = NODE_BLOCK;
    for (Token next = *peek_next_token(lexer); next.kind != TOKEN_RIGHT_CURLY && next.kind != TOKEN_END; next = *peek_next_token(lexer)) {       
        ASTNode *statement = parse_statement(lexer);
        da_append(lexer->parser->arena, statements->block, statement);
    }

    return statements;
}

b32 try_parse_parameters(Lexer *lexer) {
    if (peek_next_token(lexer)->kind != TOKEN_LEFT_PAREN) {
        return false;
    }
    eat_token(lexer);

    i32 depth = 1;

    while (depth > 0) {
        Token next = *peek_next_token(lexer);

        if (next.kind == TOKEN_END) return false;
        if (next.kind == TOKEN_LEFT_PAREN) {
            eat_token(lexer);
            depth += 1;
        }
        else if (next.kind == TOKEN_RIGHT_PAREN) {
            eat_token(lexer);
            depth -= 1;
        }
        else {
            eat_token(lexer);
        }
    }

    return true;
}

ASTNode *parse_program(Lexer *lexer) {
    ASTNode *program = push_array(lexer->parser->arena, ASTNode, 1);
    program->kind = NODE_BLOCK;

    while (peek_next_token(lexer)->kind != TOKEN_END) {
        Token next = *peek_next_token(lexer);
        parser_expect(lexer, &next, TOKEN_IDENT);
        String8 identifier = next.text;
        next = *peek_next_token(lexer);
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
        
        if (peek_next_token(lexer)->kind == TOKEN_LEFT_PAREN) {
            ASTNode *func_decl = parse_function_decl(lexer, identifier);                    
            da_append(lexer->parser->arena, program->block, func_decl);
        }
        else if (false) {} // Parse Struct
        else { // Parse global declaration
            ASTNode *global_decl = parse_var_decl(lexer, identifier);
            da_append(lexer->parser->arena, program->block, global_decl);
            next = *peek_next_token(lexer);
            parser_expect(lexer, &next, TOKEN_SEMICOLON);
        } 
    }   

    return program;
}

ASTNode *parse_decl_parameter(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    String8 param_ident = next.text;
    eat_token(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_COLON);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_IDENT);

    String8 param_type_name = next.text; 

    return make_decl_parameter(lexer->parser->arena, param_ident, param_type_name);
}

ASTNode *parse_decl_parameters(Lexer *lexer) {
    ASTNode *node = push_array(lexer->parser->arena, ASTNode, 1);
    node->kind = NODE_DECL_PARAMETER_LIST;

    Token next = *peek_next_token(lexer);

    // Function takes no parameters
    if (next.kind == TOKEN_RIGHT_PAREN) {
        node->parameter_list.size = 0;
        return node;
    }

    while (next.kind != TOKEN_RIGHT_PAREN) {
        ASTNode *parameter = parse_decl_parameter(lexer);
        da_append(lexer->parser->arena, node->parameter_list, parameter);
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_RIGHT_PAREN) {
            parser_expect(lexer, &next, TOKEN_COMMA);
        }
    }
    
    return node;
}

ASTNode *parse_function_decl(Lexer *lexer, String8 name) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_PAREN);

    ASTNode *parameter_list = {0};

    // Parse function parameters
    next = *peek_next_token(lexer);
    parameter_list = parse_decl_parameters(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);

    // Parse optional return type
    String8 return_type = str8_lit("void");
    // If main function implicitly return s32
    if (str8_compare(name, str8_lit("main"))) return_type = str8_lit("s32");
    next = *peek_next_token(lexer);
    if (next.kind == TOKEN_ARROW) {
        eat_token(lexer);
        next = *peek_next_token(lexer);
        return_type = next.text;
        parser_expect(lexer, &next, TOKEN_IDENT);
    }

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_CURLY);

    ASTNode *body = parse_statements(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);

    ASTNode *function = make_function_decl(lexer->parser->arena, name, parameter_list, body, return_type);

    return function;
}

ASTNode *parse_function_call(Lexer *lexer, String8 name) {
    ASTNode *parameter_list = parse_call_parameters(lexer);
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);

    return make_function_call(lexer->parser->arena, name, parameter_list);
}

ASTNode *parse_call_parameters(Lexer *lexer) {
    ASTNode *node = push_array(lexer->parser->arena, ASTNode, 1);
    node->kind = NODE_DECL_PARAMETER_LIST;

    Token next = *peek_next_token(lexer);

    // Function takes no parameters
    if (next.kind == TOKEN_RIGHT_PAREN) {
        node->parameter_list.size = 0;
        return node;
    }

    while (next.kind != TOKEN_RIGHT_PAREN) {
        ASTNode *parameter = parse_expression(lexer, -1);
        da_append(lexer->parser->arena, node->parameter_list, parameter);
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_RIGHT_PAREN) {
            parser_expect(lexer, &next, TOKEN_COMMA);
        }
    }
    
    return node;
}

ASTNode *parse_var_decl(Lexer *lexer, String8 name) {
    // Expect colon followed by type followed by colon, otherwise expect colon colon
    String8 var_type = str8_lit("void");
    Token next = *peek_next_token(lexer);
    if (next.kind != TOKEN_COLON_COLON) {
        parser_expect(lexer, &next, TOKEN_COLON);
        next = *peek_next_token(lexer);
        if (next.kind == TOKEN_IDENT) {
            eat_token(lexer);
            var_type = next.text;
        }
        parser_expect(lexer, &next, TOKEN_COLON);
    } else {
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
    }

    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *var_decl = make_var_decl(lexer->parser->arena, name, var_type, expr);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return var_decl;
}

ASTNode *parse_var_assign(Lexer *lexer, String8 name) {
    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *var_assign = make_var_assign(lexer->parser->arena, name, expr);
    return var_assign;
}

ASTNode *parse_if(Lexer *lexer) {
    ASTNode *cond = parse_expression(lexer, -1);
    
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_CURLY);

    ASTNode *if_body = parse_statements(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);

    ASTNode *else_body = {0};
    if (peek_token(lexer, 1)->kind == TOKEN_KEYWORD_ELSE)  {
        eat_token(lexer);
        else_body = parse_statements(lexer);
    }

    ASTNode *if_statement = make_if(lexer->parser->arena, cond, if_body, else_body);

    return if_statement;
}

ASTNode *parse_return(Lexer *lexer) {
    ASTNode *expr = parse_expression(lexer, -1);
    ASTNode *return_statement = make_return(lexer->parser->arena, expr);

    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return return_statement;
}

void parser_expect(Lexer *lexer, Token *token, TokenKind expected) {
    if (token->kind != expected) {
        print_parse_error(lexer, token, "Expected '%s'", token_kind_name(expected));
    }
    eat_token(lexer);
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

