#include "parser.h"
#include "type_check.h"

TypeKind type_kind_from_str8(String8 string) {
    if (str8_compare(string, str8_lit("s32"))) return TYPE_S32;

    fprintf(stderr, "No name defined for TypeKind %.*s\n", str8_varg(string));
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

Symbol *make_symbol(Arena *arena, String8 name, ASTNode *node, b32 is_defined) {
    Symbol *result = push_array(arena, Symbol, 1);
    result->name = name;
    result->node = node;
    result->is_defined = is_defined;
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

Scope *make_child_scope(Scope* parent_scope) {
    Scope *child = push_array(parent_scope->arena, Scope, 1);
    child->arena = parent_scope->arena;
    child->parent = parent_scope;
    child->expected_return_type = parent_scope->expected_return_type;
    child->symbols = parent_scope->symbols;
    child->types = parent_scope->types;
    return child;
}

Type *make_type_function(Arena *arena, Type **parameters, u64 parameter_count, Type *return_type) {
    Type* type = {0};
    type->kind = TYPE_FUNCTION;
    type->function.params = parameters;
    type->function.param_count = parameter_count;
    type->function.return_type = return_type;
    return type_intern(arena, GLOBAL_TYPESET, type);
}

Type *make_type_array(Arena *arena, Type* elem_type, u64 size) {
    Type *type = {0};
    type->kind = TYPE_ARRAY;
    type->array.base = elem_type;
    type->array.size = size;
    return type_intern(arena, GLOBAL_TYPESET, type);
}

Type *make_type_builtin(Arena *arena, TypeKind kind) {
    Type *type = {0};
    type->kind = kind;
    switch (kind) {
        case TYPE_S32:
            type->size = 4;
            type->align = 4;
            break;
        default:
            fprintf(stderr, "Attempted to create invalid builtin type: %.*s", str8_varg(type_kind_name(kind)));
            return (Type *){0};
    }
    return type_intern(arena, GLOBAL_TYPESET, type);
}

ResolvedExpr make_resolved_s32(Arena *arena, i64 value) {
    ResolvedExpr result = {0};
    result.value = value;
    result.is_const = true;
    result.type = make_type_builtin(arena, TYPE_S32);
    return result;
}

Symbol *resolve_name(String8 name, Scope *scope) {
    Symbol *symbol = table_get(scope->symbols, name);
    if (!symbol) {
        fprintf(stderr, "Unknown type: %.*s\n", str8_varg(name));
        return (Symbol *){0};
    }
    resolve_symbol(symbol, scope);
    return symbol;
}

Type *resolve_decl_func(ASTNode *node, Scope *scope) {
    debug_assert(node->kind == NODE_FUNCTION_DECL);
    u64 param_count = node->function_decl.parameters->parameter_list.size;
    Type **params = push_array(scope->arena, Type*, param_count);
    for (u64 i = 0; i < param_count; i++) {
        params[i] = resolve_type_syntax(node->function_decl.parameters->parameter_list.items[i]->parameter.type, scope);
    }
    Type *return_type = resolve_type_syntax(node->function_decl.return_type, scope);
    Type *type_func = make_type_function(scope->arena, params, param_count, return_type);
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
            return make_resolved_s32(scope->arena, expr->int_value);
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
            symbol->type = resolve_decl_func(symbol->node, scope);
            break;
        default:
            debug_assert(0);
            break;
    }
    symbol->state = SYMBOL_RESOLVED;
    table_insert(scope->arena, GLOBAL_ORDERED_SYMBOLS, symbol->name, symbol);
}

Type *resolve_type_syntax(TypeSyntax *type_syntax, Scope *scope) {
    switch (type_syntax->kind) {
        case TYPE_SYNTAX_NAME: {
            String8 name = type_syntax->name;
            Symbol *symbol = resolve_name(name, scope);
            if (symbol->kind != SYMBOL_TYPE) {
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
            Type *type_func = make_type_function(scope->arena, params, param_count, return_type);
            return type_func;
        }
        case TYPE_SYNTAX_ARRAY: {
            Type *elem_type = resolve_type_syntax(type_syntax->array.base, scope);
            u64 size = resolve_const_expr(type_syntax->array.size, scope);
            
            Type *type_array = make_type_array(scope->arena, elem_type, size);
            return type_array;
        }
        case TYPE_SYNTAX_POINTER: {

        }
        default:
            fprintf(stderr, "Unknown type node kind: %d", type_syntax->kind);
            return (Type*){0};
    }
}

void collect_declarations(ASTNode *decl, Scope *scope) {
    switch (decl->kind) {
    case NODE_BLOCK: {
        for (u64 i = 0; i < decl->block.size; i++) {
            // This is the declarations pass, meaning that this NODE_BLOCK is the "program" block / global block,
            // hence we dont create a child scope
            collect_declarations(decl->block.items[i], scope);
        }
        break;
    }
    case NODE_FUNCTION_DECL: {
        Symbol *symbol = table_get(scope->symbols, decl->function_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of function %*.s\n", str8_varg(decl->function_decl.name));
            } 
        } else {
            Symbol *symbol_func = make_symbol(scope->arena, decl->function_decl.name, decl, true);
            table_insert(scope->arena, &scope->symbols, decl->function_decl.name, symbol_func);
        }
        break;
    }
    case NODE_VAR_DECL: {
        Symbol *symbol = table_get(scope->symbols, decl->var_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of variable %*.s\n", str8_varg(decl->function_decl.name));
            }
        } else {
            Symbol *symbol_var = make_symbol(scope->arena, decl->var_decl.name, decl, true);
            table_insert(scope->arena, &scope->symbols, decl->var_decl.name, symbol_var);
        }
        break;
    }
    // case NODE_STRUCT_DECL: {
    //     Symbol *symbol = table_get(scope->symbols, decl->var_decl.name);
    //     if (symbol) {
    //         if (symbol->is_defined) {
    //             fprintf(stderr, "Redeclaration of struct %*.s\n", str8_varg(decl->function_decl.name));
    //         }
    //     } else {
    //         Symbol symbol = make_symbol(decl->struct_decl.name, decl, true);
    //         table_insert(scope->arena, &scope->symbols, decl->struct_decl.name, symbol);
    //         symbol.state = SYMBOL_RESOLVED;
    //         Type *type = push_array(scope->arena, Type, 1);
    //         type->kind = TYPE_INCOMPLETE;
    //         symbol.type = type;
    //     }
    //     break;
    // }
    case NODE_UNION_DECL: {
        // This case should do the exact same thing as struct decl case.
        break;
    }
    default:
        break;
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

    *set = push_array(arena, TypeSet, 1);
    (*set)->key = key; 

    return (*set)->key;
}

void dump_declarations(SymbolTable *symbols) {
    if (symbols) {
        if (symbols->value->is_defined) {
            printf("Symbol: %.*s\n", str8_varg(symbols->value->name));
        }
        for (i32 i = 0; i < 4; i++) {
            if (symbols->child[i]) {
                dump_declarations(symbols->child[i]);
            }
        }
    }
}