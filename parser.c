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

Stmt *make_stmt_expr(Arena *arena, Expr *expr) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_EXPR;
    result->expr = expr;
    return result;
}

FuncParam *make_decl_param(Arena *arena, String8 name, TypeSyntax* type) {
    FuncParam *result = push_array(arena, FuncParam, 1);
    result->name = name;
    result->type = type;
    return result;
}

FieldSyntax *make_aggregate_field(Arena *arena, String8 name, TypeSyntax* type) {
    FieldSyntax *result = push_array(arena, FieldSyntax , 1);
    result->name = name;
    result->type = type;
    return result;
}

Decl *make_decl_struct(Arena *arena, String8 struct_name, FieldSyntaxList fields) {
    Decl *result = push_array(arena, Decl, 1);
    result->kind = DECL_STRUCT;
    result->name = struct_name;
    result->aggregate.fields = fields;
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

Expr *make_expr_call(Arena *arena, Expr *expr, ExprList params) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_CALL;
    result->call.expr = expr;
    result->call.params = params; 
    return result;
}

Expr *make_expr_field(Arena *arena, Expr *expr, String8 field_name) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_FIELD;
    result->field.expr = expr;
    result->field.name = field_name;
    return result;
}

Expr *make_expr_compound(Arena *arena, ExprList fields, TypeSyntax *type) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_COMPOUND;
    result->compound_literal.fields = fields;
    result->compound_literal.type = type;
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

Expr *make_expr_name(Arena *arena, String8 name) {
    Expr *expr = push_array(arena, Expr, 1);
    expr->kind = EXPR_NAME;
    expr->name = name;
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

Stmt *make_stmt_assign(Arena *arena, Expr *lhs, Expr *rhs) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_ASSIGN; 
    result->assign_stmt.lhs = lhs;
    result->assign_stmt.rhs = rhs;
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

    Expr *result = push_array(lexer->parser->arena, Expr, 1);
    switch (next.kind) {
        case TOKEN_NAME: {
            parser_expect(lexer, &next, TOKEN_NAME);
            Expr *expr = make_expr_name(lexer->parser->arena, next.text);
            next = *peek_next_token(lexer);

            switch (next.kind) {
                case TOKEN_LEFT_PAREN: {
                    // function call
                    result = parse_expr_call(lexer, expr);
                    break;
                }
                case TOKEN_DOT: {
                    // field access
                    parser_expect(lexer, &next, TOKEN_DOT);
                    next = *peek_next_token(lexer);
                    parser_expect(lexer, &next, TOKEN_NAME);
                    String8 field_name = next.text;
                    result = make_expr_field(lexer->parser->arena, expr, field_name);
                    break;
                }
                case TOKEN_LEFT_CURLY: {
                    // Compound literal
                    TypeSyntax *type = make_type_syntax_name(lexer->parser->arena, expr->name);
                    result = parse_expr_compound(lexer, type);
                    break;
                }
                default:
                    // identifier
                    result = expr;
                    break;
            }
            break;
        }
        case TOKEN_LEFT_CURLY: {
            // Compound literal
            result = parse_expr_compound(lexer, 0);
            break;
        }
        case TOKEN_LEFT_PAREN: {
            parser_expect(lexer, &next, TOKEN_LEFT_PAREN);
            result = parse_expr(lexer, -1);
            parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_PAREN);
            break;
        }
        case TOKEN_INT: {
            parser_expect(lexer, &next, TOKEN_INT);
            result = make_s32(lexer->parser->arena, &next);
            break;
        }
        default:
            fprintf(stderr, "Error: Unexpected token '%.*s'\n", str8_varg(next.text));
            exit(EXIT_FAILURE);
    }

    return result;
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

Expr *parse_expr_compound(Lexer *lexer, TypeSyntax *type) {
    ExprList fields = {0};
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_CURLY);
    while (next.kind != TOKEN_RIGHT_CURLY) {
        Expr *field = parse_expr_compound_field(lexer);
        da_append(lexer->parser->arena, fields, field);
        next = *peek_next_token(lexer);
    }
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);
    return make_expr_compound(lexer->parser->arena, fields, type);
}

Expr *parse_expr_compound_field(Lexer *lexer) {
    Token *next = peek_next_token(lexer);
    Expr *field = parse_expr(lexer, 0);
    next = peek_next_token(lexer);
    if (next->kind != TOKEN_RIGHT_CURLY) {
        parser_expect(lexer, next, TOKEN_COMMA);
    }
    return field;
}

Stmt *parse_stmt(Lexer *lexer) {
    Token *next = peek_next_token(lexer);

    if (next->kind == TOKEN_KEYWORD_RETURN) return parse_stmt_return(lexer);
    if (next->kind == TOKEN_KEYWORD_IF)     return parse_stmt_if(lexer);

    if (next->kind == TOKEN_NAME) {
        Token *lookahead = peek_token(lexer, 1);
        if (lookahead->kind == TOKEN_COLON || lookahead->kind == TOKEN_COLON_COLON) {
            parser_expect(lexer, next, TOKEN_NAME);
            return parse_stmt_decl(lexer, next->text);
        }
    }

    Expr *lhs = parse_expr(lexer, -1);
    next = peek_next_token(lexer);
    if (next->kind == TOKEN_EQUAL) {
        return parse_stmt_assign(lexer, lhs);
    }
    return parse_stmt_expr(lexer, lhs);
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
        Token *name_token = parser_expect(lexer, peek_next_token(lexer), TOKEN_NAME);
        String8 decl_name = name_token->text;

        Token *next = peek_next_token(lexer);

        if (next->kind == TOKEN_COLON_COLON) {
            Token *lookahead = peek_token(lexer, 1);

            if (lookahead->kind == TOKEN_LEFT_PAREN) {
                Decl *func_decl = parse_decl_func(lexer, decl_name);                    
                da_append(lexer->parser->arena, program, func_decl);
            } else if (lookahead->kind == TOKEN_KEYWORD_STRUCT) {
                Decl *struct_decl = parse_decl_struct(lexer, decl_name);
                da_append(lexer->parser->arena, program, struct_decl);
            } else {
                // constant declaration "name :: expr"
                Decl *var_decl = parse_decl_var(lexer, decl_name);
                da_append(lexer->parser->arena, program, var_decl);
            }
        } else if (next->kind == TOKEN_COLON) {
            Decl *var_decl = parse_decl_var(lexer, decl_name);
            da_append(lexer->parser->arena, program, var_decl);
        } else {
            print_parse_error(lexer, next, "expected :: or :");
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
    parser_expect(lexer, peek_next_token(lexer), TOKEN_COLON_COLON);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_PAREN);

    // Parse function parameters
    Token next = *peek_next_token(lexer);
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

FieldSyntax *parse_aggregate_field(Lexer *lexer) {
    Token *next = peek_next_token(lexer);
    String8 field_name = next->text;
    eat_token(lexer);

    next = peek_next_token(lexer);
    parser_expect(lexer, next, TOKEN_COLON);

    TypeSyntax *field_type = parse_type_base(lexer); 
    next = peek_next_token(lexer);
    parser_expect(lexer, next, TOKEN_SEMICOLON);

    return make_aggregate_field(lexer->parser->arena, field_name, field_type);
}

FieldSyntaxList parse_aggregate_fields(Lexer *lexer) {
    FieldSyntaxList result = {0};
    for (Token *next = peek_next_token(lexer); next->kind != TOKEN_RIGHT_CURLY; next = peek_next_token(lexer)) {
        FieldSyntax *field = parse_aggregate_field(lexer);
        da_append(lexer->parser->arena, result, field);
    }
    return result;
}

Decl *parse_decl_struct(Lexer *lexer, String8 struct_name) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_COLON_COLON);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_STRUCT); // Struct keyword
    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY); 

    FieldSyntaxList fields = parse_aggregate_fields(lexer);

    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);

    return make_decl_struct(lexer->parser->arena, struct_name, fields);
}

Expr *parse_expr_call(Lexer *lexer, Expr *expr) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_LEFT_PAREN);
    ExprList params = parse_call_params(lexer);
    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);

    return make_expr_call(lexer->parser->arena, expr, params);
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
    TypeSyntax *var_type = push_array(lexer->parser->arena, TypeSyntax, 1);
    Token next = *peek_next_token(lexer);
    if (next.kind != TOKEN_COLON_COLON) {
        parser_expect(lexer, &next, TOKEN_COLON);
        next = *peek_next_token(lexer);
        if (next.kind == TOKEN_NAME) {
            var_type = parse_type_base(lexer);
        }
        next = *peek_next_token(lexer);
        parser_expect(lexer, &next, TOKEN_COLON);
    } else {
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
    }

    Expr *rhs = parse_expr(lexer, -1);

    Decl *var_decl = make_var_decl(lexer->parser->arena, name, var_type, rhs);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return var_decl;
}

Stmt *parse_stmt_assign(Lexer *lexer, Expr *lhs) {
    Token *next = peek_next_token(lexer);
    parser_expect(lexer, next, TOKEN_EQUAL);
    Expr *rhs = parse_expr(lexer, -1);
    next = peek_next_token(lexer);
    parser_expect(lexer, next, TOKEN_SEMICOLON);
    Stmt *var_assign = make_stmt_assign(lexer->parser->arena, lhs, rhs);
    return var_assign;
}

Stmt *parse_stmt_decl(Lexer *lexer, String8 name) {
    Stmt *result = push_array(lexer->parser->arena, Stmt, 1);
    result->kind = STMT_DECL;
    result->decl = parse_decl_var(lexer, name);
    return result;
}

Stmt *parse_stmt_if(Lexer *lexer) {
    Token next = *peek_next_token(lexer);

    parser_expect(lexer, &next, TOKEN_KEYWORD_IF);
    Expr *cond = parse_expr(lexer, -1);
    
    next = *peek_next_token(lexer);
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

Stmt *parse_stmt_expr(Lexer *lexer, Expr *expr) {
    Token *next = peek_next_token(lexer);
    parser_expect(lexer, next, TOKEN_SEMICOLON);
    return make_stmt_expr(lexer->parser->arena, expr);
}

Stmt *parse_stmt_return(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_KEYWORD_RETURN);
    Expr *expr = {0};
    next = *peek_next_token(lexer);
    if (next.kind != TOKEN_SEMICOLON) {
        expr = parse_expr(lexer, -1);
    } 
    Stmt *return_statement = make_stmt_return(lexer->parser->arena, expr);

    next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_SEMICOLON);

    return return_statement;
}

TypeSyntax *parse_type_base(Lexer *lexer) {
    Token next = *peek_next_token(lexer);
    parser_expect(lexer, &next, TOKEN_NAME);

    return make_type_syntax_name(lexer->parser->arena, next.text);
}

Token *parser_expect(Lexer *lexer, Token *token, TokenKind expected) {
    if (token->kind != expected) {
        print_parse_error(lexer, token, "Expected '%s'", token_kind_name(expected));
        return token;
    }
    eat_token(lexer);
    return token;
}

void print_parse_error(Lexer *lexer, Token *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    i32 token_length = token->end_char - token->start_char;
    fprintf(stderr, "%.*s:%ld:%d: ", str8_varg(lexer->parser->file), lexer->current_line, (int)lexer->current_char - token_length);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, " before '%.*s' \n", str8_varg(token->text));
    fprintf(stderr, "%.*s\n", (int)(lexer->input_cursor - lexer->current_char + token->end_char) , lexer->input.str);
    fprintf(stderr, "%*s^\n", (int)(lexer->current_char - token_length - 1), "");
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
        fprintf(stderr, "Error: Invalid integer format: %.*s\n", str8_varg(str));
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

void print_indent(i32 depth) {
    for (i32 i = 0; i < depth; i++) printf("  "); // 2 spaces per level
}

void ast_print_stmt(Stmt *stmt, i32 depth) {
    print_indent(depth);
    switch (stmt->kind) {
        case STMT_RETURN:
            printf("return\n");
            if (stmt->expr) {
                ast_print_expr(stmt->expr, depth + 1);
            }
            break;
        case STMT_EXPR:
            printf("stmt expr\n");
            ast_print_expr(stmt->expr, depth + 1);
            break;
        case STMT_DECL:
            printf("decl statement\n");
            ast_print_decl(stmt->decl, depth + 1);
            break;
        case STMT_ASSIGN:
            printf("assign\n");
            print_indent(depth + 1);
            printf("lhs:\n");
            ast_print_expr(stmt->assign_stmt.lhs, depth + 2);
            print_indent(depth + 1);
            printf("rhs:\n");
            ast_print_expr(stmt->assign_stmt.rhs, depth + 2);
            break;
        case STMT_IF:
            printf("if\n");
            print_indent(depth + 1);
            printf("cond:\n");
            ast_print_expr(stmt->if_stmt.cond, depth + 2);
            print_indent(depth + 1);
            printf("then:\n");
            for (u64 i = 0; i < stmt->if_stmt.if_body.size; i++)
                ast_print_stmt(stmt->if_stmt.if_body.items[i], depth + 2);
            if (stmt->if_stmt.else_body.size) {
                print_indent(depth + 1);
                printf("else:\n");
                for (u64 i = 0; i < stmt->if_stmt.else_body.size; i++)
                    ast_print_stmt(stmt->if_stmt.else_body.items[i], depth + 2);
            }
            break;
        default:
            print_indent(depth);
            printf("unknown stmt kind %d\n", stmt->kind);
            break;
    }
}

void ast_print_decl_param(FuncParam *param, i32 depth) {
    print_indent(depth);
    printf("param (%.*s: %.*s)\n", str8_varg(param->name), str8_varg(param->type->name));
}

void ast_print_type_syntax(TypeSyntax *type_syntax, i32 depth) {
    print_indent(depth);
    switch (type_syntax->kind) {
        case TYPE_SYNTAX_NAME:
            printf("type (%.*s)\n", str8_varg(type_syntax->name));
            break;
        default:
            printf("unknown type syntax %d\n", type_syntax->kind);
            break;
    }
}

void ast_print_decl(Decl *decl, i32 depth) {
    print_indent(depth);
    switch (decl->kind) {
        case DECL_VAR:
            printf("var (%.*s)\n", str8_varg(decl->name));
            ast_print_type_syntax(decl->var.type, depth + 1);
            ast_print_expr(decl->var.expr, depth + 1);
            break;
        case DECL_FUNC:
            printf("func (%.*s)\n", str8_varg(decl->name));
            ast_print_type_syntax(decl->func.return_type, depth + 1);
            for (u64 i = 0; i < decl->func.params.size; i++)
                ast_print_decl_param(decl->func.params.items[i], depth + 1);
            for (u64 i = 0; i < decl->func.body.size; i++)
                ast_print_stmt(decl->func.body.items[i], depth + 1);
            break;
        case DECL_STRUCT:
            printf("struct (%.*s)\n", str8_varg(decl->name));
            for (u64 i = 0; i < decl->aggregate.fields.size; i++) {
                print_indent(depth + 1);
                FieldSyntax *field = decl->aggregate.fields.items[i];
                printf("field (%.*s: %.*s)\n", str8_varg(field->name), str8_varg(field->type->name));
            }
            break;
        default:
            printf("unknown decl kind %d\n", decl->kind);
            break;
    }
}

void ast_print(DeclList decls, i32 depth) {
    for (u64 i = 0; i < decls.size; i++) {
        ast_print_decl(decls.items[i], depth);
    }
}

void ast_print_expr(Expr *expr, i32 depth) {
    print_indent(depth);
    switch (expr->kind) {
        case EXPR_NAME:
            printf("name (%.*s)\n", str8_varg(expr->name));
            break;
        case EXPR_INT:
            printf("int (%ld)\n", expr->int_value);
            break;
        case EXPR_BINARY:
            printf("binary (%.*s)\n", str8_varg(expr->binary.op));
            ast_print_expr(expr->binary.left, depth + 1);
            ast_print_expr(expr->binary.right, depth + 1);
            break;
        case EXPR_CALL:
            printf("call\n");
            print_indent(depth + 1);
            printf("callee:\n");
            ast_print_expr(expr->call.expr, depth + 2);
            for (u64 i = 0; i < expr->call.params.size; i++) {
                print_indent(depth + 1);
                printf("arg %llu:\n", (unsigned long long)i);
                ast_print_expr(expr->call.params.items[i], depth + 2);
            }
            break;
        case EXPR_FIELD:
            printf("field (%.*s)\n", str8_varg(expr->field.name));
            ast_print_expr(expr->field.expr, depth + 1);
            break;
        case EXPR_COMPOUND:
            printf("compound literal\n");
            for (u64 i = 0; i < expr->compound_literal.fields.size; i++) {
                ast_print_expr(expr->compound_literal.fields.items[i], depth + 1);
            }
            break;
        default:
            print_indent(depth);
            printf("unknown expr kind %d\n", expr->kind);
            break;
    }
}