#include "lexer.h"
#include "parser.h"
#include "base/base_include.h"

TokenKind to_operator(Token *token) {
    return token->kind;
}

b32 is_binary_operator(Token *token) {
    switch (token->kind) {
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_PLUS:
        case TOKEN_MULT:
        case TOKEN_GT:
        case TOKEN_LT:
            return true;
        default:
            return false;
    }
}

i32 get_precedence_from_kind(TokenKind kind) {
    return token_literals[kind].precedence;
}

// Make nodes
Expr *make_literal_from_token(Arena  *arena, Token *token) {
    Expr *result = push_array(arena, Expr, 1);
    result->kind = EXPR_LITERAL,
    result->tav.mode = ADRESSING_CONSTANT,
    result->tav.value = value_from_token(token);
    return result;
}

Expr *make_expr_binary(Arena *arena, Expr *left, TokenKind operator, Expr *right) {
    Expr *result = push_array(arena, Expr, 1);
    *result = (Expr){ 
        .kind = EXPR_BINARY, 
        .binary.left = left, 
        .binary.op = operator, 
        .binary.right = right 
    };
    return result;
}

Expr *make_expr_unary(Arena *arena, TokenKind operator, Expr *operand) {
    Expr *result = push_array(arena, Expr, 1);
    *result = (Expr){
        .kind = EXPR_UNARY,
        .unary.op = operator,
        .unary.expr = operand,
    };
    return result;
}

Stmt *make_stmt_return(Arena *arena, Expr *expr) {
    Stmt *result = push_array(arena, Stmt, 1);
    *result = (Stmt){ 
        .kind = STMT_RETURN,
        .expr = expr 
    };
    return result;
}

Stmt *make_stmt_expr(Arena *arena, Expr *expr) {
    Stmt *result = push_array(arena, Stmt, 1);
    *result = (Stmt){
        .kind = STMT_EXPR,
        .expr = expr
    };
    return result;

}

FuncParam *make_decl_param(Arena *arena, String8 name, TypeSyntax* type) {
    FuncParam *result = push_array(arena, FuncParam, 1);
    *result = (FuncParam){
        .name = name,
        .type = type
    };
    return result;
}

FieldSyntax *make_aggregate_field(Arena *arena, String8 name, TypeSyntax* type) {
    FieldSyntax *result = push_array(arena, FieldSyntax , 1);
    *result = (FieldSyntax){
        .name = name,
        .type = type
    };
    return result;
}

Decl *make_decl_struct(Arena *arena, String8 struct_name, FieldSyntaxList fields) {
    Decl *result = push_array(arena, Decl, 1);
    *result = (Decl){
        .kind = DECL_STRUCT, 
        .name = struct_name,
        .aggregate.fields = fields,
    };
    return result;
}

Decl *make_decl_func(Arena *arena, String8 name, ParamList params, StmtList body, TypeSyntax *return_type, b32 is_external_func) {
    Decl *result = push_array(arena, Decl, 1);
    *result = (Decl){
        .kind = DECL_FUNC, 
        .name = name, 
        .linkage = is_external_func ? LINKAGE_EXTERNAL : LINKAGE_INTERNAL,
        .func.params = params, 
        .func.body = body, 
        .func.return_type = return_type,
    };
    return result;
}

Expr *make_expr_call(Arena *arena, Expr *expr, ExprList params) {
    Expr *result = push_array(arena, Expr, 1);
    *result = (Expr){
        .kind = EXPR_CALL,
        .call.expr = expr,
        .call.params = params,
    };
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

Stmt *make_stmt_while(Arena *arena, Expr *cond, StmtList body) {
    Stmt *result = push_array(arena, Stmt, 1);
    result->kind = STMT_WHILE;
    result->while_stmt.cond = cond;
    result->while_stmt.body = body;
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

TypeSyntax *make_type_syntax_ptr(Arena *arena, TypeSyntax *type) {
    TypeSyntax *result = push_array(arena, TypeSyntax, 1);
    result->kind = TYPE_SYNTAX_POINTER;
    result->ptr.base = type;
    return result;
}

TypeSyntax *make_type_syntax_array(Arena *arena, TypeSyntax *type, Expr *size_expr) {
    TypeSyntax *result = push_array(arena, TypeSyntax, 1);
    result->kind = TYPE_SYNTAX_ARRAY;
    result->array.base = type;
    result->array.size = size_expr;
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
                    Token *next = parser_expect(lexer, peek_next_token(lexer), TOKEN_NAME);
                    String8 field_name = next->text;
                    result = make_expr_field(lexer->parser->arena, expr, field_name);
                    break;
                }
                case TOKEN_LEFT_CURLY: {
                    // Compound literal
                    // TODO: If you accidently type 'Struct' instead of 'struct', 
                    // the parser thinks we are creating compound literal with a type qualifier, 
                    // resulting in a non-sensical parsing error.
                    if (lexer->parser->context == CONTEXT_CONDITION) {
                        result = expr;
                        break;
                    }

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
        case TOKEN_AND: {
            Token *operator = parser_expect(lexer, &next, TOKEN_AND);
            Expr *operand = parse_leaf(lexer);

            result = make_expr_unary(lexer->parser->arena, operator->kind, operand);
            break;
        };
        case TOKEN_STRING: {
            parser_expect(lexer, &next, TOKEN_STRING);
            result = make_literal_from_token(lexer->parser->arena, &next);
            break;
        }
        case TOKEN_INT: {
            parser_expect(lexer, &next, TOKEN_INT);
            result = make_literal_from_token(lexer->parser->arena, &next);
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
    Token next = *parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY);
    while (next.kind != TOKEN_RIGHT_CURLY) {
        Expr *field = parse_expr_compound_field(lexer);
        da_append(lexer->parser->arena, fields, field);
        next = *peek_next_token(lexer);
    }
    parser_expect(lexer, &next, TOKEN_RIGHT_CURLY);
    return make_expr_compound(lexer->parser->arena, fields, type);
}

Expr *parse_expr_compound_field(Lexer *lexer) {
    Expr *field = parse_expr(lexer, 0);
    Token *next = peek_next_token(lexer);
    if (next->kind != TOKEN_RIGHT_CURLY) {
        parser_expect(lexer, next, TOKEN_COMMA);
    }
    return field;
}

Stmt *parse_stmt(Lexer *lexer) {
    Token *next = peek_next_token(lexer);

    if (next->kind == TOKEN_KEYWORD_RETURN) return parse_stmt_return(lexer);
    if (next->kind == TOKEN_KEYWORD_IF)     return parse_stmt_if(lexer);
    if (next->kind == TOKEN_KEYWORD_WHILE)  return parse_stmt_while(lexer);

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
            parser_expect(lexer, next, TOKEN_COLON_COLON);
            
            next = peek_next_token(lexer);
            b32 is_extern_function = (next->kind == TOKEN_KEYWORD_EXTERN);
            if (next->kind == TOKEN_LEFT_PAREN || is_extern_function) {
                Decl *func_decl = parse_decl_func(lexer, decl_name, is_extern_function);                    
                da_append(lexer->parser->arena, program, func_decl);
            } else if (next->kind == TOKEN_KEYWORD_STRUCT) {
                Decl *struct_decl = parse_decl_struct(lexer, decl_name);
                da_append(lexer->parser->arena, program, struct_decl);
            } else {
                // global variable declaration "name :: expr" (Should eval to a constant)
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

    parser_expect(lexer, peek_next_token(lexer), TOKEN_COLON);

    TypeSyntax *param_type = parse_type(lexer, 0); 

    return make_decl_param(lexer->parser->arena, param_name, param_type);
}

ParamList parse_decl_params(Lexer *lexer) {
    ParamList result = {0};

    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_PAREN);

    Token next = *peek_next_token(lexer);

    // Function takes no parameters
    if (next.kind == TOKEN_RIGHT_PAREN) {
        parser_expect(lexer, &next, TOKEN_RIGHT_PAREN);
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

    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_PAREN);
    
    return result;
}

Decl *parse_decl_func(Lexer *lexer, String8 name, b32 is_extern_function) {

    if(is_extern_function) {
        parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_EXTERN);
    }

    // Parse function parameters
    ParamList params = parse_decl_params(lexer);

    // Parse optional return type
    TypeSyntax *return_type = make_type_syntax_name(lexer->parser->arena, str8_lit("void"));
    // If main function implicitly return s32
    if (str8_compare(name, str8_lit("main"))) {
        return_type = make_type_syntax_name(lexer->parser->arena, str8_lit("s32"));
    }
    Token next = *peek_next_token(lexer);
    if (next.kind == TOKEN_ARROW) {
        parser_expect(lexer, &next, TOKEN_ARROW);
        return_type = parse_type(lexer, 0);
    }

    next = *peek_next_token(lexer);

    StmtList body = {0};
    if (!is_extern_function) {
        parser_expect(lexer, &next, TOKEN_LEFT_CURLY);
        body = parse_stmts(lexer);
        parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);
    } else {
        // If function is declared external, we expect a semicolon, and there shouldn't be a body
        parser_expect(lexer, &next, TOKEN_SEMICOLON);
    }
    
    Decl *function = make_decl_func(lexer->parser->arena, name, params, body, return_type, is_extern_function);

    return function;
}

FieldSyntax *parse_aggregate_field(Lexer *lexer) {
    Token *next = parser_expect(lexer, peek_next_token(lexer), TOKEN_NAME);
    String8 field_name = next->text;

    parser_expect(lexer, peek_next_token(lexer), TOKEN_COLON);

    TypeSyntax *field_type = parse_type(lexer, 0); 
    parser_expect(lexer, peek_next_token(lexer), TOKEN_SEMICOLON);

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
    parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_STRUCT); 
    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY); 

    FieldSyntaxList fields = parse_aggregate_fields(lexer);

    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);

    return make_decl_struct(lexer->parser->arena, struct_name, fields);
}

Expr *parse_expr_call(Lexer *lexer, Expr *expr) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_PAREN);
    ExprList params = parse_call_params(lexer);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_PAREN);

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
        var_type = parse_type(lexer, 0);
        parser_expect(lexer, peek_next_token(lexer), TOKEN_COLON);
    } else {
        parser_expect(lexer, &next, TOKEN_COLON_COLON);
    }

    Expr *rhs = parse_expr(lexer, -1);

    Decl *var_decl = make_var_decl(lexer->parser->arena, name, var_type, rhs);

    parser_expect(lexer, peek_next_token(lexer), TOKEN_SEMICOLON);

    return var_decl;
}

Stmt *parse_stmt_while(Lexer *lexer) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_WHILE);
    lexer->parser->context = CONTEXT_CONDITION;
    Expr *cond = parse_expr(lexer, -1);
    lexer->parser->context = CONTEXT_DEFAULT;

    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY);
    StmtList body = parse_stmts(lexer);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);

    return make_stmt_while(lexer->parser->arena, cond, body);
}

Stmt *parse_stmt_assign(Lexer *lexer, Expr *lhs) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_EQUAL);
    Expr *rhs = parse_expr(lexer, -1);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_SEMICOLON);
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
    parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_IF);

    lexer->parser->context = CONTEXT_CONDITION;
    Expr *cond = parse_expr(lexer, -1);
    lexer->parser->context = CONTEXT_DEFAULT;

    parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY);
    StmtList if_body = parse_stmts(lexer);
    parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);

    StmtList else_body = {0};
    Token *next = peek_next_token(lexer);
    if (next->kind == TOKEN_KEYWORD_ELSE)  {
        parser_expect(lexer, next, TOKEN_KEYWORD_ELSE);
        parser_expect(lexer, peek_next_token(lexer), TOKEN_LEFT_CURLY);
        else_body = parse_stmts(lexer);
        parser_expect(lexer, peek_next_token(lexer), TOKEN_RIGHT_CURLY);
    }

    Stmt *if_statement = make_stmt_if(lexer->parser->arena, cond, if_body, else_body);

    return if_statement;
}

Stmt *parse_stmt_expr(Lexer *lexer, Expr *expr) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_SEMICOLON);
    return make_stmt_expr(lexer->parser->arena, expr);
}

Stmt *parse_stmt_return(Lexer *lexer) {
    parser_expect(lexer, peek_next_token(lexer), TOKEN_KEYWORD_RETURN);
    Expr *expr = {0};
    Token next = *peek_next_token(lexer);
    if (next.kind != TOKEN_SEMICOLON) {
        expr = parse_expr(lexer, -1);
    } 
    Stmt *return_statement = make_stmt_return(lexer->parser->arena, expr);

    parser_expect(lexer, peek_next_token(lexer), TOKEN_SEMICOLON);

    return return_statement;
}

TypeSyntax *parse_type(Lexer *lexer, TypeSyntax **type) {
    Token next = *peek_next_token(lexer);
    if (!type) {
        type = push_array(lexer->parser->arena, TypeSyntax*, 1);
    }
    if (next.kind == TOKEN_NAME) {
        Token *name = parser_expect(lexer, &next, TOKEN_NAME);
        TypeSyntax *base_type = make_type_syntax_name(lexer->parser->arena, name->text);
        *type = base_type;
        return *type;
    }
    if (next.kind == TOKEN_MULT) {
        parser_expect(lexer, &next, TOKEN_MULT);
        TypeSyntax *ptr_type = make_type_syntax_ptr(lexer->parser->arena, *type);
        *type = ptr_type;
        parse_type(lexer, &(*type)->ptr.base);
    }
    if (next.kind == TOKEN_LEFT_BRACKET) {
        parser_expect(lexer, &next, TOKEN_LEFT_BRACKET);
        Expr *size_expr = {0};
        next = *peek_next_token(lexer);
        if (next.kind != TOKEN_RIGHT_BRACKET) {
            size_expr = parse_expr(lexer, -1);
        }
        parser_expect(lexer, &next, TOKEN_RIGHT_BRACKET);
        TypeSyntax *array_type = make_type_syntax_array(lexer->parser->arena, *type, size_expr);
        *type = array_type;
        parse_type(lexer, &(*type)->array.base);
    }

    return *type; 
}

inline Token *parser_expect(Lexer *lexer, Token *token, TokenKind expected) {
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
