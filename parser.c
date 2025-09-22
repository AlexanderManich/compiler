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

void ast_print_stmt(Stmt *stmt, i32 depth) {
    printf("%*s", depth, "");
    switch (stmt->kind) {
        case STMT_RETURN: 
            printf("return ");
            ast_print_expr(stmt->expr, depth + 2);
            break;
        default:
            debug_assert(0);
            break;
    }
}

void ast_print_decl_param(FuncParam *param) {
    printf("decl param (%.*s: %.*s)\n", str8_varg(param->name), str8_varg(param->type->name));
}

void ast_print_type_syntax(TypeSyntax *type_syntax) {
    switch (type_syntax->kind) {
        case TYPE_SYNTAX_NAME:
            printf("%.*s\n", str8_varg(type_syntax->name));
            break;
        default:
            debug_assert(0);
            break;
    }
}

void ast_print_expr(Expr *expr, i32 depth) {
    debug_assert(expr);
    switch (expr->kind) {
        case EXPR_NAME:
            printf("(%.*s)\n", str8_varg(expr->name));
            break;
        case EXPR_INT:
            printf("integer (%ld)\n", expr->int_value);
            break;
        case EXPR_CALL:
            printf("call (%.*s)\n", str8_varg(expr->call.name));
            for (u64 i = 0; i < expr->call.params.size; i++) {
                printf("%*s", depth, "");
                printf("call param ");
                ast_print_expr(expr->call.params.items[i], depth + 2);
            }
            break;
        default:
            debug_assert(0);
            break;
    }
}

void ast_print_decl(Decl *decl, i32 depth) {
    debug_assert(decl);
    switch (decl->kind) {
        case DECL_VAR: 
            ast_print_expr(decl->var.expr, depth + 2);
            ast_print_type_syntax(decl->var.type);
            break;
        case DECL_FUNC:
            printf("decl func (%.*s)", str8_varg(decl->name));
            printf(" -> ");
            ast_print_type_syntax(decl->func.return_type);
            for (u64 i = 0; i < decl->func.params.size; i++) {
                printf("%*s", depth, "");
                ast_print_decl_param(decl->func.params.items[i]);
            }
            for (u64 i = 0; i < decl->func.body.size; i++ ) {
                ast_print_stmt(decl->func.body.items[i], depth + 2);
            }
            break;
        default:
            debug_assert(0);
            break;
    }
}

void ast_print(DeclList decls, i32 depth) {
    if (!decls.size) return;
    printf("%*s", depth, "");
    
    for (u64 i = 0; i < decls.size; i++) {
        ast_print_decl(decls.items[i], depth + 2);
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
        case NODE_INT:
            return str8_lit("integer");
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
Expr *make_s32(Arena *arena, Token *token) {
    // TODO: This needs fixing later. Not sure how to handle different sizes of int. Obviously we can't know while parsing.
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_INT;
    result->int_value = i32_from_str8(token->text);
    return result;
}

Expr *make_expr_binary(Arena *arena, Expr *left, String8 operator, Expr *right) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_BINARY;
    result->binary.left = left;
    result->binary.op = operator;
    result->binary.right = right;
    return result;
}

Stmt *make_stmt_return(Arena *arena, Expr *expr) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_RETURN;
    result->expr = expr;
    return result;
}

FuncParam *make_decl_param(Arena *arena, String8 name, TypeSyntax* type) {
    FuncParam *result = push_array(arena, FuncParam, 1);
    result->name = name;
    result->type = type;
    return result;
}

Decl *make_decl_func(Arena *arena, String8 name, ParamList params, StmtList body, TypeSyntax *return_type) {
    Decl *result = push_array(arena, Decl, 1);
    result->kind = DECL_FUNC;
    result->name = name;
    result->func.params = params;
    result->func.body = body;
    result->func.return_type = return_type;
    return result;
}

Expr *make_expr_call(Arena *arena, String8 name, ExprList params) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_CALL;
    result->call.name = name;
    result->call.params = params; 
    return result;
}

Stmt *make_stmt_if(Arena *arena, Expr *cond, StmtList if_body, StmtList else_body) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_IF;
    result->if_stmt.cond = cond;
    result->if_stmt.if_body = if_body;
    result->if_stmt.else_body = else_body;
    return result;
}

Expr *make_expr_name(Arena *arena, Token *token) {
    Expr *expr = push_array(arena, Expr, 1);
    expr->kind = EXPR_NAME;
    expr->name = token->text;
    return expr;
}

Decl *make_var_decl(Arena *arena, String8 name, TypeSyntax *type, Expr *expr) {
    Decl *result = push_array(arena, Decl, 1);
    result->kind = DECL_VAR;
    result->name = name;
    result->var.type = type;
    result->var.expr = expr;
    return result;
}

Stmt *make_stmt_assign(Arena *arena, String8 name, Expr *expr) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_ASSIGN; 
    result->assign_stmt.name = name;
    result->assign_stmt.expr = expr;
    return result;
}

TypeSyntax *make_type_syntax_name(Arena *arena, String8 name) {
    TypeSyntax *result = push_array(arena, TypeSyntax, 1);
    result->kind = TYPE_SYNTAX_NAME;
    result->name = name;
    return result;
}

// Parsing
Expr *parse_leaf(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    eat_token(lexer);

    Expr *result = push_array(lexer->parser->arena, Expr, 1);
    switch (next.kind) {
        case TOKEN_NAME: {
            String8 identifier = next.text;
            if (peek_next_token(lexer)->kind == TOKEN_LEFT_PAREN) {
                eat_token(lexer);
                result = parse_expr_call(lexer, identifier);
            } else {
                result = make_expr_name(lexer->parser->arena, &next);
            }
            return result;
        }
        case TOKEN_INT: {
            result = make_s32(lexer->parser->arena, &next);
            return result;
        }
        default:
            fprintf(stderr, "Error: Unexpected token '%.*s'\n", str8_varg(next.text));
            exit(EXIT_FAILURE);
    }
}

Expr *parse_increasing_precedence(Lexer *lexer, Expr *left, i32 min_precedence) {
    Token next = *peek_next_token(lexer);

    if (!is_binary_operator(&next)) return left;

    i32 next_prec = get_precedence_from_kind(next.kind);

    if (min_precedence <= next_prec) {
        eat_token(lexer);
        Expr *right = parse_expr(lexer, next_prec);
        return make_expr_binary(lexer->parser->arena, left, to_operator(&next), right);
    }
    else {
        return left;
    }
}

Expr *parse_expr(Lexer *lexer, i32 min_precedence) {
    Expr *left = parse_leaf(lexer);
    while (true) {
        Expr *expr = parse_increasing_precedence(lexer, left, min_precedence);
        if (expr == left) break;
        left = expr;
    }
    
    return left;
}

Stmt *parse_stmt(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    eat_token(lexer);

    Stmt *statement = {0};
    
    if (next.kind == TOKEN_KEYWORD_IF) {
        statement = parse_stmt_if(lexer);
    }     
    if (next.kind == TOKEN_NAME) {
        String8 name = next.text;

        Token next = *peek_next_token(lexer);
        if (next.kind == TOKEN_COLON || next.kind == TOKEN_COLON_COLON) {
            statement = parse_stmt_decl(lexer, name);
        } else if (next.kind == TOKEN_EQUAL) {
            statement = parse_stmt_assign(lexer, name);
        }
    }
    if (next.kind == TOKEN_KEYWORD_RETURN) {
        statement = parse_stmt_return(lexer);
    }

    return statement;
}

StmtList parse_stmts(Lexer *lexer) {
    StmtList result = {0};
    for (Token next = *peek_next_token(lexer); next.kind != TOKEN_RIGHT_CURLY && next.kind != TOKEN_END; next = *peek_next_token(lexer)) {       
        Stmt *stmt = parse_stmt(lexer);
        da_append(lexer->parser->arena, result, stmt);
    }

    return result;
}

DeclList parse_program(Lexer *lexer) {
    DeclList program = {0};

    while (peek_next_token(lexer)->kind != TOKEN_END) {
        Token next = *peek_next_token(lexer);
        parser_expect(lexer, &next, TOKEN_NAME);
        String8 decl_name = next.text;
        next = *peek_next_token(lexer);
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
        
        if (peek_next_token(lexer)->kind == TOKEN_LEFT_PAREN) {
            Decl *func_decl = parse_decl_func(lexer, decl_name);                    
            da_append(lexer->parser->arena, program, func_decl);
        }
        else if (false) {} // Parse Struct
        else { // Parse global variable declarations
            Decl *var_decl = parse_decl_var(lexer, decl_name);
            da_append(lexer->parser->arena, program, var_decl);
            next = *peek_next_token(lexer);
            parser_expect(lexer, &next, TOKEN_SEMICOLON);
        } 
    }   

    return program;
}

FuncParam *parse_decl_param(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    String8 param_name = next.text;
    eat_token(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_COLON);

    TypeSyntax *param_type = parse_type_base(lexer); 

    return make_decl_param(lexer->parser->arena, param_name, param_type);
}

ParamList parse_decl_params(Lexer *lexer) {
    ParamList result = {0};

    Token next = *peek_next_token(lexer);

    // Function takes no parameters
    if (next.kind == TOKEN_RIGHT_PAREN) {
        result.size = 0;
        return result;
    }

    while (next.kind != TOKEN_RIGHT_PAREN) {
        FuncParam *param = parse_decl_param(lexer);
        da_append(lexer->parser->arena, result, param);
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_RIGHT_PAREN) {
            parser_expect(lexer, &next, TOKEN_COMMA);
        }
    }
    
    return result;
}

Decl *parse_decl_func(Lexer *lexer, String8 name) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_PAREN);

    // Parse function parameters
    next = *peek_next_token(lexer);
    ParamList params = parse_decl_params(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);

    // Parse optional return type
    TypeSyntax *return_type = make_type_syntax_name(lexer->parser->arena, str8_lit("void"));
    // If main function implicitly return s32
    if (str8_compare(name, str8_lit("main"))) {
        return_type = make_type_syntax_name(lexer->parser->arena, str8_lit("s32"));
    }
    next = *peek_next_token(lexer);
    if (next.kind == TOKEN_ARROW) {
        eat_token(lexer);
        return_type = parse_type_base(lexer);
    }

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_CURLY);

    StmtList body = parse_stmts(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);

    Decl *function = make_decl_func(lexer->parser->arena, name, params, body, return_type);

    return function;
}

Expr *parse_expr_call(Lexer *lexer, String8 name) {
    ExprList params = parse_call_params(lexer);
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);

    return make_expr_call(lexer->parser->arena, name, params);
}

ExprList parse_call_params(Lexer *lexer) {
    ExprList result = {0};

    Token next = *peek_next_token(lexer);

    // Function takes no parameters
    if (next.kind == TOKEN_RIGHT_PAREN) {
        result.size = 0;        
        return result;
    }

    while (next.kind != TOKEN_RIGHT_PAREN) {
        Expr *param = parse_expr(lexer, -1);
        da_append(lexer->parser->arena, result, param);
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_RIGHT_PAREN) {
            parser_expect(lexer, &next, TOKEN_COMMA);
        }
    }
    
    return result;
}

Decl *parse_decl_var(Lexer *lexer, String8 name) {
    // Expect colon followed by type followed by colon, otherwise expect colon colon
    TypeSyntax *var_type = make_type_syntax_name(lexer->parser->arena, str8_lit("void"));
    Token next = *peek_next_token(lexer);
    if (next.kind != TOKEN_COLON_COLON) {
        parser_expect(lexer, &next, TOKEN_COLON);
        next = *peek_next_token(lexer);
        if (next.kind == TOKEN_NAME) {
            var_type = parse_type_base(lexer);
        }
        parser_expect(lexer, &next, TOKEN_COLON);
    } else {
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
    }

    Expr *expr = parse_expr(lexer, -1);
    Decl *var_decl = make_var_decl(lexer->parser->arena, name, var_type, expr);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return var_decl;
}

Stmt *parse_stmt_assign(Lexer *lexer, String8 name) {
    Expr *expr = parse_expr(lexer, -1);
    Stmt *var_assign = make_stmt_assign(lexer->parser->arena, name, expr);
    return var_assign;
}

Stmt *parse_stmt_decl(Lexer *lexer, String8 name) {
    Stmt *result = push_array(lexer->parser->arena, Stmt, 1);
    result->kind = STMT_DECL;
    result->decl = parse_decl_var(lexer, name);
    return result;
}

Stmt *parse_stmt_if(Lexer *lexer) {
    Expr *cond = parse_expr(lexer, -1);
    
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_CURLY);

    StmtList if_body = parse_stmts(lexer);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);

    StmtList else_body = {0};
    if (peek_token(lexer, 1)->kind == TOKEN_KEYWORD_ELSE)  {
        eat_token(lexer);
        else_body = parse_stmts(lexer);
    }

    Stmt *if_statement = make_stmt_if(lexer->parser->arena, cond, if_body, else_body);

    return if_statement;
}

Stmt *parse_stmt_return(Lexer *lexer) {
    Expr *expr = parse_expr(lexer, -1);
    Stmt *return_statement = make_stmt_return(lexer->parser->arena, expr);

    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return return_statement;
}

TypeSyntax *parse_type_base(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_NAME);

    return make_type_syntax_name(lexer->parser->arena, next.text);
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

