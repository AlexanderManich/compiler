#include "parser.h"
#include "type_check.h"

static Arena GLOBAL_TYPE_ARENA;

TypeKind type_kind_from_str8(String8 string) {
    if (str8_compare(string, str8_lit("void"))) return TYPE_VOID;
    if (str8_compare(string, str8_lit("s32"))) return TYPE_S32;

    fprintf(stderr, "No TypeKind defined for name: %.*s\n", str8_varg(string));
    return TYPE_VOID;
}

String8 type_kind_name(TypeKind kind) {
    switch(kind) {
        case TYPE_VOID: return str8_lit("void");
        case TYPE_S32: return str8_lit("s32");
        default: fprintf(stderr, "No name defined for TypeKind %d\n", kind);
    }
    return (String8){0};
}

String8 expr_kind_name(ExprKind kind) {
    switch(kind) {
        case EXPR_NAME:     return str8_lit("expr_name");
        case EXPR_UNARY:    return str8_lit("expr_unary");
        case EXPR_BINARY:   return str8_lit("expr_binary");
        case EXPR_COMPOUND: return str8_lit("expr_compound");
        case EXPR_INT:      return str8_lit("expr_int");
        case EXPR_FIELD:    return str8_lit("expr_field");
        case EXPR_CALL:     return str8_lit("expr_call");
        default: fprintf(stderr, "No name defined for ExprKind %d\n", kind);
    }
    return (String8){0};
}

String8 stmt_kind_name(StmtKind kind) {
    switch(kind) {
        case STMT_BLOCK:  return str8_lit("stmt_block");
        case STMT_DECL:   return str8_lit("stmt_decl");
        case STMT_RETURN: return str8_lit("stmt_return");
        case STMT_EXPR:   return str8_lit("stmt_expr");
        case STMT_WHILE:  return str8_lit("stmt_while");
        case STMT_ASSIGN: return str8_lit("stmt_assign");
        case STMT_IF:     return str8_lit("stmt_if");
        case STMT_NONE:   return str8_lit("stmt_none");
        default: fprintf(stderr, "No name defined for StmtKind %d\n", kind);
    }
    return (String8){0};
}

String8 name_of_type(Type *type) {
    debug_assert(type->symbol);
    return type->symbol->name;
}

Symbol *make_symbol(Arena *arena, SymbolKind kind, Decl *decl) {
    Symbol *result = push_array(arena, Symbol, 1);
    result->name = decl->name;
    result->decl = decl;
    result->kind = kind;
    return result;
}

Symbol *make_symbol_var(Arena *arena, String8 name, Type *type) {
    Symbol *result = push_array(arena, Symbol, 1);
    result->name = name;
    result->type = type;
    result->kind = SYMBOL_VAR;
    result->state = SYMBOL_RESOLVED;
    return result;
}

Scope *make_child_scope(Arena *local_arena, Scope* parent_scope) {
    Scope *child = push_array(parent_scope->arena, Scope, 1);
    child->arena = local_arena;
    child->parent = parent_scope;
    child->expected_return_type = parent_scope->expected_return_type;
    return child;
}

Type *make_type_function(Type **parameters, u64 parameter_count, Type *return_type) {
    Type type = {0};
    type.kind = TYPE_FUNCTION;
    type.function.params = parameters;
    type.function.param_count = parameter_count;
    type.function.return_type = return_type;
    return type_intern(&GLOBAL_TYPE_ARENA, &GLOBAL_TYPE_TABLE, &type);
}

Type *make_type_array(Type* elem_type, u64 size) {
    Type type = {0};
    type.kind = TYPE_ARRAY;
    type.array.base = elem_type;
    type.array.size = size;
    return type_intern(&GLOBAL_TYPE_ARENA, &GLOBAL_TYPE_TABLE, &type);
}

Type *make_type_ptr(Type* base_type) {
    // TODO: Make defines for ptr size and align
    Type type = { 
        .kind = TYPE_PTR, 
        .size = 8,
        .align = 8,
        .ptr.base = base_type
    };
    return type_intern(&GLOBAL_TYPE_ARENA, &GLOBAL_TYPE_TABLE, &type);
}

Type *make_type_incomplete(Arena *arena, Symbol *symbol) {
    Type *type = push_array(arena, Type, 1);
    type->kind = TYPE_INCOMPLETE;
    type->symbol = symbol;
    return type; 
}

ResolvedExpr make_resolved_s32(i64 value) {
    ResolvedExpr result = {0};
    result.value = value;
    result.is_const = true;
    result.type = builtin_symbols[TYPE_S32].type;
    return result;
}

Symbol *resolve_name(String8 name, Scope *scope) {
    Symbol *symbol = scope_get_symbol(scope, name);
    if (!symbol) {
        fprintf(stderr, "Undeclared symbol: %.*s\n", str8_varg(name));
        return (Symbol *){0};
    }
    resolve_symbol(symbol, scope);
    return symbol;
}

Type *resolve_decl_func(Decl *decl, Scope *scope) {
    debug_assert(decl->kind == DECL_FUNC);
    u64 param_count = decl->func.params.size;
    Type **params = push_array(scope->arena, Type*, param_count);
    for (u64 i = 0; i < param_count; i++) {
        params[i] = resolve_type_syntax(decl->func.params.items[i]->type, scope);
    }
    Type *return_type = resolve_type_syntax(decl->func.return_type, scope);
    Type *type_func = make_type_function(params, param_count, return_type);
    return type_func;
}

i64 resolve_const_expr(Expr *expr, Scope *scope) {
    // TODO: I dont know about this. Added it in the case of unspecified size of array, since expr * is 0 then.
    if (!expr) {
        return 0;
    }
    ResolvedExpr result = resolve_expr(expr, scope);
    if (!result.is_const) {
        fprintf(stderr, "Expected a constant expression");
        return 0;
    }
    return result.value;
}

ResolvedExpr resolved_rvalue(Type *type) {
    ResolvedExpr result = { .type = type };
    return result;
}

ResolvedExpr resolved_lvalue(Type *type) {
    ResolvedExpr result = { .type = type, .is_lvalue = true, };
    return result;
}

ResolvedExpr resolved_const(u64 value) {
    ResolvedExpr result = { .type = builtin_symbols[TYPE_S32].type, .is_const = true, .value = value };
    return result;
}

ResolvedExpr resolve_expr_call(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_CALL);
    ResolvedExpr called = resolve_expr(expr->call.expr, scope);
    if (called.type->kind != TYPE_FUNCTION) {
        fprintf(stderr, "Tried calling non-function: %.*s\n", str8_varg(expr->name));
        debug_assert(0);
    }
    if (expr->call.params.size != called.type->function.param_count) {
        fprintf(stderr, "Mismatched number of parameters. Function %.*s expected %lu paramters, but was called with %lu\n",
            str8_varg(called.type->symbol->name), called.type->function.param_count, expr->call.params.size);
        debug_assert(0);
    }

    for (u64 i = 0; i < expr->call.params.size; i++) {
        Expr *param = expr->call.params.items[i];
        Type *expected_param_type = called.type->function.params[i];
        scope->expected_return_type = expected_param_type;
        ResolvedExpr resolved_param = resolve_expr(param, scope);
        if (expected_param_type != resolved_param.type) {
            fprintf(stderr, "Type mismatch when attempting to call function %.*s. " 
                "Parameter %.*s has type %.*s, but parameter of type %.*s was expected",
                str8_varg(expr->name), str8_varg(param->name), 
                str8_varg(type_kind_name(resolved_param.type->kind)), str8_varg(type_kind_name(expected_param_type->kind)));
        }
    }
    return resolved_rvalue(called.type->function.return_type);
}

ResolvedExpr resolve_expr_name(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_NAME);
    Symbol *symbol = resolve_name(expr->name, scope);
    if (symbol->kind == SYMBOL_VAR) {
        expr->symbol = symbol;
        return resolved_lvalue(symbol->type);
    }
    if (symbol->kind == SYMBOL_CONST) {
        return resolved_const(symbol->value);
    }
    if (symbol->kind == SYMBOL_FUNC) {
        return resolved_rvalue(symbol->type);
    }
    return (ResolvedExpr){0};
}

u64 eval_int_binary(u64 left_val, TokenKind op, u64 right_val) {
    if (op == TOKEN_PLUS) return left_val + right_val;

    return 0;
}

ResolvedExpr resolve_expr_binary(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_BINARY);
    ResolvedExpr left = resolve_expr(expr->binary.left, scope);
    ResolvedExpr right = resolve_expr(expr->binary.right, scope);
    // TODO: Check if any numerical type is match, not just s32 
    if (left.type != builtin_symbols[TYPE_S32].type) {
        fprintf(stderr, "left operand of + must be of type s32");
    }
    if (left.type != right.type) {
        fprintf(stderr, "Type mismatch - %.*s %s %.*s", 
            str8_varg(name_of_type(left.type)), token_kind_name(expr->binary.op).str, str8_varg(name_of_type(right.type)));
    }
    if (left.is_const && right.is_const) {
        u64 expr_result = eval_int_binary(left.value, expr->binary.op, right.value);
        return resolved_const(expr_result);
    } else {
        return resolved_rvalue(left.type);
    }
}

ResolvedExpr resolve_expr_unary(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_UNARY);
    ResolvedExpr operand = resolve_expr(expr->unary.expr, scope);
    Type *type = operand.type;
    switch (expr->unary.op) {
        case TOKEN_AND:
            if (!operand.is_lvalue) eprintf("Attempted to take address of non-lvalue\n");
            Type *ptr_type = make_type_ptr(type);
            return resolved_rvalue(ptr_type);
        default:
            debug_assert(0);
    }
}

ResolvedExpr resolve_expr_compound(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_COMPOUND);
    if (!scope->expected_decl_type && !expr->compound_literal.type) {
        fprintf(stderr, "Compound literal used in initialization with no explicit type\n");
        debug_assert(0);
    }
    Type *type = {0};
    if (expr->compound_literal.type) {
        type = resolve_type_syntax(expr->compound_literal.type, scope);
    } else {
        type = scope->expected_decl_type;
    }
    complete_type(type, scope);

    if (type->kind != TYPE_STRUCT && type->kind != TYPE_UNION && type->kind != TYPE_ARRAY) {
        fprintf(stderr, "Attempted to initialize non aggregate or array type with compound literal\n");
        debug_assert(0);
    }

    if (type->kind == TYPE_STRUCT || type->kind == TYPE_UNION) {
        ExprList fields = expr->compound_literal.fields;
        for (u64 i = 0; i < fields.size; i++) {
            Expr *field = fields.items[i];
            resolve_expr(field, scope);
        }
    } else if (type->kind == TYPE_ARRAY) {
        // TODO:
    } else {
        debug_assert(0);
    }
    return resolved_rvalue(type);
}

ResolvedExpr resolve_expr_field(Expr *expr, Scope *scope) {
    debug_assert(expr->kind == EXPR_FIELD);
    ResolvedExpr accessee = resolve_expr(expr->field.expr, scope);
    Type *accessee_type = accessee.type;
    complete_type(accessee_type, scope);
    if (accessee_type->kind != TYPE_STRUCT && accessee_type->kind != TYPE_UNION) {
        fprintf(stderr, "Attempted to access field on non-aggregate type\n");
        return (ResolvedExpr){0};
    }
    for (u64 i = 0; i < accessee_type->aggregate.fields.size; i++) {
        Field field = accessee_type->aggregate.fields.items[i];
        if (str8_compare(field.name, expr->field.name)) {
            return accessee.is_lvalue ? resolved_lvalue(field.type) : resolved_rvalue(field.type);
        }
    }

    fprintf(stderr, "No field named %.*s on type %.*s", str8_varg(expr->field.name), str8_varg(expr->field.expr->name));

    return (ResolvedExpr){0};
}

ResolvedExpr resolve_expr(Expr *expr, Scope *scope) {
    switch (expr->kind) {
        case EXPR_NAME: 
            return resolve_expr_name(expr, scope);
        case EXPR_INT: 
            // TODO: Built in types should be allocated in a static arena, not a locally scoped one
            return make_resolved_s32(expr->int_value);
        case EXPR_UNARY:
            return resolve_expr_unary(expr, scope);
        case EXPR_BINARY:
            return resolve_expr_binary(expr, scope);
        case EXPR_CALL:
            return resolve_expr_call(expr, scope);
        case EXPR_COMPOUND:
            return resolve_expr_compound(expr, scope);
        case EXPR_FIELD:
            return resolve_expr_field(expr, scope);
        default:
            eprintf("Failed at resolving expression: ");
            ast_print_expr(expr, 0);
            return (ResolvedExpr){0};
    }
}

void resolve_symbol(Symbol *symbol, Scope *scope) {
    if (symbol->state == SYMBOL_RESOLVED) {
        return;
    } else if (symbol->state == SYMBOL_RESOLVING) {
        fprintf(stderr, "Detected cyclic dependency while resolving symbol: %.*s", str8_varg(symbol->name));
        return;
    }
    symbol->state =  SYMBOL_RESOLVING;
    switch (symbol->kind) {
        // case SYMBOL_TYPE:
        //     NOTE: resolve typedef
        //     symbol->type = resolve_decl_type(symbol->node);
        //     break;
        // case SYMBOL_VAR:
        //     symbol->type = resolve_decl_var(symbol->decl);
        //     break;
        // case SYMBOL_CONST:
        //     symbol->type = resolve_decl_const(symbol->node);
        //     break;
        case SYMBOL_FUNC:
            symbol->type = resolve_decl_func(symbol->decl, scope);
            symbol->type->symbol = symbol;
            break;
        default:
            debug_assert(0);
            break;
    }
    symbol->state = SYMBOL_RESOLVED;
    da_append(scope->arena, GLOBAL_ORDERED_SYMBOLS, symbol);
    // table_insert(scope->arena, &GLOBAL_ORDERED_SYMBOLS, symbol->name, symbol);
}

Type *resolve_type_syntax(TypeSyntax *type_syntax, Scope *scope) {
    if (!type_syntax) return (Type *){0};

    switch (type_syntax->kind) {
        case TYPE_SYNTAX_NAME: {
            String8 name = type_syntax->name;
            Symbol *symbol = resolve_name(name, scope);
            if (!symbol || symbol->kind != SYMBOL_TYPE) {
                fprintf(stderr, "%.*s doesn't denote a type\n", str8_varg(name));
                return (Type *){0};
            }
            return symbol->type;
        }
        case TYPE_SYNTAX_FUNCTION: {
            u64 param_count = type_syntax->function.param_count;
            Type **params = push_array(scope->arena, Type*, param_count);
            for (u64 i = 0; i < param_count; i++) {
                params[i] = resolve_type_syntax(type_syntax->function.params[i], scope);
            }
            Type *return_type = resolve_type_syntax(type_syntax->function.return_type, scope);
            Type *type_func = make_type_function(params, param_count, return_type);
            return type_func;
        }
        case TYPE_SYNTAX_ARRAY: {
            Type *elem_type = resolve_type_syntax(type_syntax->array.base, scope);
            u64 size = resolve_const_expr(type_syntax->array.size, scope);
            
            Type *type_array = make_type_array(elem_type, size);
            return type_array;
        }
        case TYPE_SYNTAX_POINTER: {
            Type *base_type = resolve_type_syntax(type_syntax->ptr.base, scope);
            return make_type_ptr(base_type);
        }
        default:
            fprintf(stderr, "Unknown type node kind: %d\n", type_syntax->kind);
            return (Type*){0};
    }
}

void register_decl(Decl *decl, Scope *scope) {
    Symbol *symbol = scope_get_symbol(scope, decl->name);
    if (symbol) {
        fprintf(stderr, "Redeclaration of symbol %*.s\n", str8_varg(decl->name));
        return;
    }
    SymbolKind kind = SYMBOL_NONE;
    switch (decl->kind) {
        case DECL_VAR: 
            kind = SYMBOL_VAR;
            break;
        case DECL_FUNC:
            kind = SYMBOL_FUNC;
            break;
        case DECL_STRUCT:
        case DECL_UNION:
        case DECL_ENUM:
            kind = SYMBOL_TYPE;
            break;
        default:
            debug_assert(0);
            break;
    }

    symbol = make_symbol(scope->arena, kind, decl);
    if (decl->kind == DECL_STRUCT || decl->kind == DECL_UNION) {
        symbol->state = SYMBOL_RESOLVED;
        symbol->type = make_type_incomplete(scope->arena, symbol);
    }
    table_insert(scope->arena, &scope->symbols, symbol->name, symbol);
}



void register_builtin_symbols(Scope *global_scope) {
    for (u64 i = 0; i < TYPE_NUMERICAL_END; i++) {
        Symbol *symbol = &builtin_symbols[i];
        symbol->type->symbol = symbol;
        table_insert(global_scope->arena, &global_scope->symbols, symbol->name, symbol);
    }
}

void register_decls(DeclList decls, Scope *scope) {
    for (u64 i = 0; i < decls.size; i++) {
        register_decl(decls.items[i], scope);
    }
}

void complete_struct_type(Type *type, FieldList fields) {
    debug_assert(type->kind == TYPE_COMPLETING);
    type->kind = TYPE_STRUCT;
    for (u64 i = 0; i < fields.size; i++) {
        // TODO: Add padding
        type->size = fields.items[i].type->size + type->size;
        type->align = max(fields.items[i].type->align, type->align);
    }
    type->aggregate.fields = fields;
}

// void complete_union_type(Type *type, FieldList fields) {
    // TODO:
// }

void complete_type(Type *type, Scope *scope) {
    if (type->kind == TYPE_COMPLETING) {
        fprintf(stderr, "Cyclic type dependency while trying to complete type: %.*s", str8_varg(type->symbol->name));
    }
    if (type->kind != TYPE_INCOMPLETE) {
        return;
    }
    type->kind = TYPE_COMPLETING;
    Decl *decl = type->symbol->decl;
    debug_assert(decl->kind == DECL_STRUCT || decl->kind == DECL_UNION);
    FieldList fields = {0};

    for (u64 i = 0; i < decl->aggregate.fields.size; i++) {
        FieldSyntax *field_syntax = decl->aggregate.fields.items[i];
        Type *field_type = resolve_type_syntax(field_syntax->type, scope);
        complete_type(field_type, scope);
        Field field = { .name = field_syntax->name, .type = field_type };
        da_append(scope->arena, fields, field);
    }

    String8 type_name = type->symbol->name;

    if (fields.size == 0) {
        fprintf(stderr, "Aggregate type %.*s has no fields\n", str8_varg(type_name));
        debug_assert(0);
    }

    if (fields_have_duplicates(fields)) {
        fprintf(stderr, "Duplicate fields in aggregate %.*s\n", str8_varg(type_name));
    }

    if (decl->kind == DECL_STRUCT) {
        complete_struct_type(type, fields);
    }
    if (decl->kind == DECL_UNION) {
        // complete_union_type(type, fields);
    }
    
    da_append(scope->arena, GLOBAL_ORDERED_SYMBOLS, type->symbol);
    // table_insert(scope->arena, &GLOBAL_ORDERED_SYMBOLS, type->symbol->name, type->symbol);
}

void resolve_func_body(Symbol *symbol, Scope *local_scope) {
    debug_assert(symbol->state == SYMBOL_RESOLVED);
    Decl *decl = symbol->decl;
    debug_assert(decl->kind == DECL_FUNC);
    u64 params_count = decl->func.params.size;
    symbol->func.params = push_array(local_scope->arena, Symbol *, params_count);
    symbol->func.params_count = params_count;
    for (u64 i = 0; i < params_count; i++) {
        FuncParam *param = decl->func.params.items[i];
        Type* param_type = resolve_type_syntax(param->type, local_scope);
        Symbol *param_symbol = make_symbol_var(local_scope->arena, param->name, param_type);
        table_insert(local_scope->arena, &local_scope->symbols, param_symbol->name, param_symbol);
        symbol->func.params[i] = param_symbol;
    }
    // NOTE: Overwrite expected return type, to match return type of the function we have entered
    local_scope->expected_return_type = resolve_type_syntax(decl->func.return_type, local_scope);
    resolve_stmt_block(decl->func.body, local_scope);
}

void resolve_stmt_decl(Decl *decl, Scope *scope) {
    switch (decl->kind) {
        case DECL_VAR: {
            Type *var_type = {0};
            if (decl->var.type->name.str) {
                var_type = resolve_type_syntax(decl->var.type, scope);
            }
            scope->expected_decl_type = var_type;
            Type *expr_type = resolve_expr(decl->var.expr, scope).type;
    
            if (var_type && var_type != expr_type) {
                printf("Expression type doesn't match declared type - Decl: ");
                print_type(var_type);
                printf(", Expr: ");
                print_type(expr_type);
                printf("\n");
                return;
            }
                
            Symbol *symbol = make_symbol_var(scope->arena, decl->name, expr_type);
            // Link symbol to declaration. Used in codegen
            decl->symbol = symbol;
            symbol->decl = decl;
            table_insert(scope->arena, &scope->symbols, decl->name, symbol);
            break;
        }
        case DECL_FUNC:
            break;
        default:
            debug_assert(0);
            break;
    }
}

void resolve_stmt_assign(Stmt *stmt, Scope *scope) {
    debug_assert(stmt->kind == STMT_ASSIGN);
    ResolvedExpr lhs = resolve_expr(stmt->assign_stmt.lhs, scope);
    ResolvedExpr rhs = resolve_expr(stmt->assign_stmt.rhs, scope);

    if (lhs.type != rhs.type) {
        fprintf(stderr, "Trying to assign rhs of type %.*s, to lhs of type %.*s\n", 
            str8_varg(type_kind_name(rhs.type->kind)), str8_varg(type_kind_name(lhs.type->kind)));
        return;
    }

    if (!lhs.is_lvalue) {
        fprintf(stderr, "Trying to assign to non lvalue lhs\n");
        return;
    }

    if (lhs.is_const) {
        fprintf(stderr, "Lhs is const and cannot be modified\n");
        return;
    }
}

void resolve_stmt_if(Stmt *stmt, Scope *scope) {
    debug_assert(stmt->kind == STMT_IF);
    ResolvedExpr cond = resolve_expr(stmt->if_stmt.cond, scope);

    if (cond.type != builtin_symbols[TYPE_S32].type) fprintf(stderr, "If condition must evaluate to S32\n");
    if (stmt->if_stmt.if_body.size > 0) {
        resolve_stmt_block(stmt->if_stmt.if_body, scope);
    }
    if (stmt->if_stmt.else_body.size > 0) {
        resolve_stmt_block(stmt->if_stmt.else_body, scope);
    }
}

void resolve_stmt_while(Stmt *stmt, Scope *scope) {
    debug_assert(stmt->kind == STMT_WHILE);
    ResolvedExpr cond = resolve_expr(stmt->if_stmt.cond, scope);

    if (cond.type != builtin_symbols[TYPE_S32].type) fprintf(stderr, "While condition must evaluate to S32\n");
    if (stmt->while_stmt.body.size > 0) {
        resolve_stmt_block(stmt->if_stmt.if_body, scope);
    }
}

void resolve_stmt(Stmt *stmt, Scope *scope) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            // TODO: 
            if (stmt->expr) {
                ResolvedExpr expr = resolve_expr(stmt->expr, scope);
                if (expr.type != scope->expected_return_type) {
                    fprintf(stderr, "Mismatched return type. Expected %.*s got %.*s\n",
                        str8_varg(type_kind_name(scope->expected_return_type->kind)), str8_varg(type_kind_name(expr.type->kind)));
                    return;
                }
            }
            break;
        }
        case STMT_DECL:
            resolve_stmt_decl(stmt->decl, scope);
            break;
        case STMT_ASSIGN:
            resolve_stmt_assign(stmt, scope);
            break;
        case STMT_EXPR:
            resolve_expr(stmt->expr, scope);
            break;
        case STMT_IF:
            resolve_stmt_if(stmt, scope);
            break;
        case STMT_WHILE:
            resolve_stmt_while(stmt, scope);
            break;
        default:
            debug_assert(0);
            break;
    }
}

void resolve_stmt_block(StmtList block, Scope *scope) {
    // NOTE: I dont bother to make a temp arena for any generic block, 
    // i just use the temp arena space that the function scope already holds.
    Scope *block_scope = make_child_scope(scope->arena, scope);
    for (u64 i = 0; i < block.size; i++) {
        resolve_stmt(block.items[i], block_scope);
    }
}

void resolve_symbols(SymbolTable *symbols, Scope *scope) {
    if (symbols) {
        Symbol *symbol = symbols->value;
        resolve_symbol(symbol, scope);
        if (symbol->kind == SYMBOL_TYPE) {
            complete_type(symbol->type, scope);
        }
        if (symbol->kind == SYMBOL_FUNC) {
            Scope *func_scope = make_child_scope(scope->arena, scope);
            resolve_func_body(symbol, func_scope);
        }
        for (u64 i = 0; i < array_count(symbols->child); i++){
            if (symbols->child[i]) {
                resolve_symbols(symbols->child[i], scope);
            }
        }
    }
}

b32 fields_have_duplicates(FieldList fields) {
    for (u64 i = 0; i < fields.size; i++) {
        for (u64 j = i+1; j < fields.size; j++) {
            if (str8_compare(fields.items[i].name, fields.items[j].name)) return true;
        }
    }
    return false;
}

// https://nullprogram.com/blog/2023/09/30/ 
u64 str8_hash(String8 key) {
    u64 hash = 0x100;
    for (u64 i = 0; i < key.size; i++) {
        hash ^= key.str[i];
        hash *= 1111111111111111111u;
    }
    return hash;
}

b32 table_insert(Arena *arena, SymbolTable **table, String8 key, Symbol *value) {
    for (u64 h = str8_hash(key); *table; h <<=2) {
        if (str8_compare(key, (*table)->key)) {
            (*table)->value = value;
            return false;
        }
        table = &(*table)->child[h >> 62];
    }
    // Symbol *value_storage = push_array(arena, Symbol, 1);
    // value_storage = value;

    *table = push_array(arena, SymbolTable, 1);
    (*table)->key = key;
    // (*table)->value = value_storage;
    (*table)->value = value;
    return true;
}

Symbol *scope_get_symbol(Scope *scope, String8 key) {
    for (; scope; scope = scope->parent) {
        Symbol *symbol = table_get(scope->symbols, key);
        if (symbol) return symbol;
    }
    return (Symbol *){0};
}

Symbol *table_get(SymbolTable *table, String8 key) {
    for (u64 h = str8_hash(key); table; h <<= 2) {
        if (str8_compare(key, table->key)) {
            return table->value;
        }
        table = table->child[h >> 62];
    }
    return (Symbol *){0};
}

u64 type_hash(Type *type) {
    u64 h = (u64)type->kind;

    switch (type->kind) {
        case TYPE_FUNCTION: {
            h ^= type->function.param_count;
            for (u64 i = 0; i < type->function.param_count; i++) {
                h ^= (uintptr_t)type->function.params[i] >> 3;
                h *= 0x9e3779b97f4a7c15;
            }
            h ^= ((uintptr_t)type->function.return_type >> 3);
            break;
        }

        default:
            return (u64)type->kind;
            break;
    }

    return 0;
}

b32 type_equals(Type *a, Type *b) {
    if (!(a && b)) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
        case TYPE_FUNCTION:
            if (a->function.param_count != b->function.param_count) return false;
            if (a->function.return_type != b->function.return_type) return false;
            for (u64 i = 0; i < a->function.param_count; i++) {
                if (a->function.params[i] != b->function.params[i]) return false;
            }
            return true;
        case TYPE_PTR:
            return type_equals(a->ptr.base, b->ptr.base);
        default:
        // If we end up in this case, the type should be interned, and we can do pointer comparison
            return a == b;
    }
}

Type *type_intern(Arena *arena, TypeSet **set, Type *key) {
    for (u64 h = type_hash(key); *set; h <<= 2) {
        if (type_equals(key, (*set)->key)) {
            return (*set)->key;
        }
        set = &(*set)->child[h>>62];
    }

    Type *key_storage = push_array(arena, Type, 1);
    *key_storage = *key;

    *set = push_array(arena, TypeSet, 1);
    (*set)->key = key_storage; 

    return key_storage;
}

void symbol_print_decls(SymbolTable *symbols) {
    if (symbols) {
        printf("Symbol: %.*s", str8_varg(symbols->value->name));
        printf(", state: %d\n", symbols->value->state);
        for (u64 i = 0; i < array_count(symbols->child); i++) {
            if (symbols->child[i]) {
                symbol_print_decls(symbols->child[i]);
            }
        }
    }
}

void print_type(Type *type) {
    if (!type) {
        printf("<null>");
        return;
    }
    switch (type->kind) {
        case TYPE_VOID:
        case TYPE_S32:
            printf("%.*s", str8_varg(type_kind_name(type->kind)));
            break;
        case TYPE_PTR:
            printf("*");
            print_type(type->ptr.base);
            break;
        case TYPE_ARRAY:
            printf("[%ld]", type->array.size);
            print_type(type->array.base);
            break;
        case TYPE_FUNCTION:
            printf("(");
            for (u64 i = 0; i < type->function.param_count; i++) {
                print_type(type->function.params[i]);
                if (i + 1 < type->function.param_count) printf(", ");
            }
            printf(") -> ");
            print_type(type->function.return_type);
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            if (type->symbol)
                printf("%.*s", str8_varg(type->symbol->name));
            else
                printf("<anon struct/union>");
            break;
        default:
            printf("<unknown type>");
            break;
    }
}