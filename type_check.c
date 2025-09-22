#include "parser.h"
#include "type_check.h"

TypeKind type_kind_from_str8(String8 string) {
    if (str8_compare(string, str8_lit("void"))) return TYPE_VOID;
    if (str8_compare(string, str8_lit("s32"))) return TYPE_S32;

    fprintf(stderr, "No name defined for TypeKind: %.*s\n", str8_varg(string));
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

TypeSyntax *make_type_syntax_function(Arena *arena, ASTNode *node) {
    TypeSyntax *type_node = push_array(arena, TypeSyntax, 1);
    type_node->kind = TYPE_SYNTAX_FUNCTION;
    
    ASTNode *param_list = node->function_decl.parameters;
    u64 param_count = param_list->parameter_list.size;
    
    type_node->function.params = push_array(arena, TypeSyntax*, param_count);
    type_node->function.param_count = param_count;

    for (u64 i = 0; i < param_count; i++) {
        ASTNode *param_node = param_list->parameter_list.items[i];
        type_node->function.params[i] = param_node->parameter.type;
    }

    type_node->function.return_type = node->function_decl.return_type;

    return type_node;
}

Scope *make_child_scope(Arena *local_arena, Scope* parent_scope) {
    Scope *child = push_array(parent_scope->arena, Scope, 1);
    child->arena = local_arena;
    child->parent = parent_scope;
    child->expected_return_type = parent_scope->expected_return_type;
    return child;
}

Type *make_type_function(Scope *scope, Type **parameters, u64 parameter_count, Type *return_type) {
    Type* type = {0};
    type->kind = TYPE_FUNCTION;
    type->function.params = parameters;
    type->function.param_count = parameter_count;
    type->function.return_type = return_type;
    return type_intern(scope->arena, &scope->types, type);
}

Type *make_type_array(Scope *scope, Type* elem_type, u64 size) {
    Type *type = {0};
    type->kind = TYPE_ARRAY;
    type->array.base = elem_type;
    type->array.size = size;
    return type_intern(scope->arena, &scope->types, type);
}

Type *make_type_builtin(Scope *scope, TypeKind kind) {
    Type type = {0};
    type.kind = kind;
    switch (kind) {
        case TYPE_VOID:
            break;
        case TYPE_S32:
            type.size = 4;
            type.align = 4;
            break;
        default:
            fprintf(stderr, "Attempted to create invalid builtin type: %.*s\n", str8_varg(type_kind_name(kind)));
            return (Type *){0};
    }
    return type_intern(scope->arena, &scope->types, &type);
}

Type *make_type_incomplete(Arena *arena, Symbol *symbol) {
    Type *type = push_array(arena, Type, 1);
    type->kind = TYPE_INCOMPLETE;
    type->symbol = symbol;
    return type; 
}

ResolvedExpr make_resolved_s32(Scope *scope, i64 value) {
    ResolvedExpr result = {0};
    result.value = value;
    result.is_const = true;
    result.type = make_type_builtin(scope, TYPE_S32);
    return result;
}

Symbol *resolve_name(String8 name, Scope *scope) {
    Symbol *symbol = scope_get_symbol(scope, name);
    if (!symbol) {
        fprintf(stderr, "Failed while resolving name: %.*s\n", str8_varg(name));
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
    Type *type_func = make_type_function(scope, params, param_count, return_type);
    return type_func;
}

i64 resolve_const_expr(Expr *expr, Scope *scope) {
    ResolvedExpr result = resolve_expr(expr, scope);
    if (!result.is_const) {
        fprintf(stderr, "Expected a constant expression");
        return 0;
    }
    return result.value;
}

ResolvedExpr resolve_expr(Expr *expr, Scope *scope) {
    switch (expr->kind) {
        case EXPR_INT: 
            // TODO: Built in types should be allocated in a static arena, not a locally scoped one
            return make_resolved_s32(scope, expr->int_value);
        default:
            // fprintf(stderr, "Failed at resolving expression kind: %.*s", str8_varg(node_kind_name(expr)));
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
        //     symbol->type = resolve_decl_var(symbol->node);
        //     break;
        // case SYMBOL_CONST:
        //     symbol->type = resolve_decl_const(symbol->node);
        //     break;
        case SYMBOL_FUNC:
            symbol->type = resolve_decl_func(symbol->decl, scope);
            break;
        default:
            debug_assert(0);
            break;
    }
    symbol->state = SYMBOL_RESOLVED;
    table_insert(scope->arena, &GLOBAL_ORDERED_SYMBOLS, symbol->name, symbol);
}

Type *resolve_type_syntax(TypeSyntax *type_syntax, Scope *scope) {
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
            Type *type_func = make_type_function(scope, params, param_count, return_type);
            return type_func;
        }
        case TYPE_SYNTAX_ARRAY: {
            Type *elem_type = resolve_type_syntax(type_syntax->array.base, scope);
            u64 size = resolve_const_expr(type_syntax->array.size, scope);
            
            Type *type_array = make_type_array(scope, elem_type, size);
            return type_array;
        }
        case TYPE_SYNTAX_POINTER: {

        }
        default:
            fprintf(stderr, "Unknown type node kind: %d", type_syntax->kind);
            return (Type*){0};
    }
}

void register_decl(Decl *decl, Scope *scope) {
    Symbol *symbol = table_get(scope->symbols, decl->name);
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
        default:
            debug_assert(0);
            break;
    }

    symbol = make_symbol(scope->arena, kind, decl);
    if (decl->kind == DECL_STRUCT || DECL_UNION) {
        symbol->state = SYMBOL_RESOLVED;
        symbol->type = make_type_incomplete(scope->arena, symbol);
    }
    table_insert(scope->arena, &scope->symbols, symbol->name, symbol);
}

static Symbol builtin_symbols[] = {
    { .name = str8_lit_comp("void"), .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED },
    { .name = str8_lit_comp("s32"),  .kind = SYMBOL_TYPE, .state = SYMBOL_RESOLVED },
};

void register_builtin_symbols(Scope *global_scope) {
    for (u64 i = 0; i < array_count(builtin_symbols); i++) {
        Symbol symbol = builtin_symbols[i];
        TypeKind type_kind = type_kind_from_str8(symbol.name);
        // This also interns the inbuilt types
        symbol.type = make_type_builtin(global_scope, type_kind);
        table_insert(global_scope->arena, &global_scope->symbols, symbol.name, &symbol);
    }
}

void register_decls(DeclList decls, Scope *scope) {
    for (u64 i = 0; i < decls.size; i++) {
        register_decl(decls.items[i], scope);
    }
}

void complete_type(Type *type) {

}

void resolve_func_body(Symbol *symbol, Scope *local_scope) {
    debug_assert(symbol->state == SYMBOL_RESOLVED);
    Decl *decl = symbol->decl;
    debug_assert(decl->kind == DECL_FUNC);

    for (u64 i = 0; i < decl->func.params.size; i++) {
        FuncParam *param = decl->func.params.items[i];
        Type* param_type = resolve_type_syntax(param->type, local_scope);
        Symbol *var = make_symbol_var(local_scope->arena, param->name, param_type);
        table_insert(local_scope->arena, &local_scope->symbols, var->name, var);
    }
    // NOTE: Overwrite expected return type, to match return type of the function we have entered
    local_scope->expected_return_type = resolve_type_syntax(decl->func.return_type, local_scope);
    resolve_stmt_block(decl->func.body, local_scope);
}

void resolve_stmt(Stmt *stmt, Scope *scope) {
    switch (stmt->kind) {
        case STMT_RETURN:
            
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
            complete_type(symbol->type);
        }
        if (symbol->kind == SYMBOL_FUNC) {
            TempArena temp = temp_begin(scope->arena);
            Scope *func_scope = make_child_scope(temp.arena, scope);
            resolve_func_body(symbol, func_scope);
            temp_end(temp);
        }
        for (u64 i = 0; i < array_count(symbols->child); i++){
            if (symbols->child[i]) {
                resolve_symbols(symbols->child[i], scope);
            }
        }
    }
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
    Symbol *value_storage = push_array(arena, Symbol, 1);
    *value_storage = *value;

    *table = push_array(arena, SymbolTable, 1);
    (*table)->key = key;
    (*table)->value = value_storage;
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
    if (a->kind == TYPE_FUNCTION) {
        if (a->function.param_count != b->function.param_count) return false;
        if (a->function.return_type != b->function.return_type) return false;

        for (u64 i = 0; i < a->function.param_count; i++) {
            if (a->function.params[i] != b->function.params[i]) return false;
        }
    }

    return true;
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
        printf("Symbol: %.*s\n", str8_varg(symbols->value->name));
        for (u64 i = 0; i < array_count(symbols->child); i++) {
            if (symbols->child[i]) {
                symbol_print_decls(symbols->child[i]);
            }
        }
    }
}