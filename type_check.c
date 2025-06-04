#include "parser.h"
#include "type_check.h"

TypeKind type_kind_from_str8(String8 string) {
    if (str8_compare(string, str8_lit("s32"))) return TYPE_S32;
    return TYPE_VOID;
}

String8 str8_from_type_kind(TypeKind kind) {
    switch(kind) {
        case TYPE_VOID: return str8_lit("void");
        case TYPE_S32: return str8_lit("s32");
        default: fprintf(stderr, "No name defined for TypeKind %d\n", kind);
    }
    return (String8){0};
}

b32 type_equals(Type *a, Type *b) {
    debug_assert(a && b);
    if (a == b) return true;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case TYPE_S32:
            return true;
        case TYPE_FUNCTION:
            if (a->params_count != b->params_count) return false;

            for (u64 i = 0; i < a->params_count; i++) {
                if (!type_equals(a->params[i], b->params[i])) return false;
            }

            return type_equals(a->return_type, b->return_type);
        default:
            return false;
    }
}

Type *make_simple_type(Arena *arena, TypeKind kind) {
    Type *result = push_array(arena, Type, 1);
    result->kind = kind;
    return result;
}

Type *make_function_type(Arena *arena, ASTNode *node) {
    Type* result = push_array(arena, Type, 1);
    result->kind = TYPE_FUNCTION;

    ASTNode *param_list = node->function_decl.parameters;
    u64 param_count = param_list->parameter_list.size;
    result->params_count = param_count;

    result->params = push_array(arena, Type*, param_count);

    for (u64 i = 0; i < param_count; i++) {
        ASTNode *param_node = param_list->parameter_list.items[i];
        Type *param_type = push_array(arena, Type, 1);
        param_type->kind = param_node->parameter.type->kind; 
        result->params[i] = param_type;
    }

    result->return_type = node->function_decl.return_type;
    return result;
}

Type *make_expr_type(ASTNode *expr, Scope *scope) {
    switch (expr->kind) {
        case NODE_CONST_INT: {
            return make_simple_type(scope->arena, TYPE_S32);
        }
        default:
        break;
    }
    return (Type *){0};
}

Scope *make_child_scope(Scope* parent_scope) {
    Scope *child = push_array(parent_scope->arena, Scope, 1);
    child->arena = parent_scope->arena;
    child->parent = parent_scope;
    child->expected_return_type = parent_scope->expected_return_type;
    return child;
}

void collect_declarations(ASTNode *node, Scope *scope) {
    switch (node->kind) {
    case NODE_BLOCK: {
        for (u64 i = 0; i < node->block.size; i++) {
            // This is the declarations pass, meaning that this NODE_BLOCK is the "program" block / global block,
            // hence we dont create a child scope
            collect_declarations(node->block.items[i], scope);
        }
        break;
    }
    case NODE_FUNCTION_DECL: {
        Symbol *symbol = map_get(scope->symbols, node->function_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of function %*.s\n", str8_varg(node->function_decl.name));
                exit(1);
            } 
        } else {
            Type *func_type = make_function_type(scope->arena, node);
            Symbol func_symbol = {
                .identifier = node->function_decl.name,               
                .type = func_type, 
                .is_defined = true
            };
            map_insert(scope->arena, &scope->symbols, node->function_decl.name, func_symbol);
        }
        break;
    }
    case NODE_VAR_DECL: {
        Symbol *symbol = map_get(scope->symbols, node->var_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of variable %*.s\n", str8_varg(node->function_decl.name));
                exit(1);
            }
        } else {
            Symbol var_symbol = {
                .identifier = node->var_decl.name,
                .type = node->var_decl.type,
                .is_defined = true
            };
            map_insert(scope->arena, &scope->symbols, node->var_decl.name, var_symbol);
        }
        break;
    }
        default:
            break;
    }
}

void type_check_program(ASTNode *root_node, Scope *global_scope) {
    debug_assert(root_node->block.items);
    for (u64 i = 0; i < root_node->block.size; i++) {
        type_check_node(root_node->block.items[i], global_scope);
    }
}

void type_check_node(ASTNode *node, Scope *scope) {
    switch (node->kind) {
        case NODE_BLOCK: {
            Scope *block_scope = make_child_scope(scope);
            for (u64 i = 0; i < node->block.size; i++) {
                type_check_node(node->block.items[i], block_scope);
            }
            break;
        }
        case NODE_FUNCTION_DECL: {
            Symbol *function_symbol = map_get(scope->symbols, node->function_decl.name);
            debug_assert(function_symbol);

            Scope *function_scope = make_child_scope(scope);
            function_scope->expected_return_type = node->function_decl.return_type;

            // Add function parameters to function scope
            for (u64 i = 0; i < node->function_decl.parameters->parameter_list.size; i++) {
                ASTNode *parameter = node->function_decl.parameters->parameter_list.items[i];
                Symbol parameter_symbol = {
                    .identifier = parameter->parameter.name,
                    .type = parameter->parameter.type,
                    .is_defined = true,
                };
                map_insert(function_scope->arena, &function_scope->symbols, parameter->parameter.name, parameter_symbol);
            }

            type_check_node(node->function_decl.body, function_scope);
            break;
        }
        case NODE_RETURN: {
            Type *expr_type = make_expr_type(node->return_val.expr, scope);
            Type *expected_return_type = scope->expected_return_type;
            
            if (!type_equals(expr_type, expected_return_type)) {
                fprintf(stderr, "Mismatched return type. Expected: %s\n", str8_from_type_kind(expected_return_type->kind).str);
                exit(1);
            }
            break;
        }
        default:
            fprintf(stderr, "Unexpected Node while type checking: %s\n", node_kind_name(node).str);
            break;
    }
}

// https://nullprogram.com/blog/2023/09/30/ 
u64 hash(String8 key) {
    u64 hash = 0x100;
    for (u64 i = 0; i < key.size; i++) {
        hash ^= key.str[i];
        hash *= 1111111111111111111u;
    }
    return hash;
}

b32 equals(String8 a, String8 b) {
    return a.size == b.size && !memcmp(a.str, b.str, a.size);
}

b32 map_insert(Arena *arena, SymbolTable **table, String8 key, Symbol value) {
    for (u64 h = hash(key); *table; h <<=2) {
        if (equals(key, (*table)->key)) {
            (*table)->value = value;
            return false;
        }
        table = &(*table)->child[h >> 62];
    }
    *table = push_array(arena, SymbolTable, 1);
    (*table)->key = key;
    (*table)->value = value;
    return true;
}

Symbol *map_get(SymbolTable *table, String8 key) {
    for (u64 h = hash(key); table; h <<=2) {
        if (equals(key, table->key)) {
            return &table->value;
        }
        table = table->child[h >> 62];
    }
    return (Symbol *){0};
}