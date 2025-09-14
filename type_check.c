#include "parser.h"
#include "type_check.h"

TypeKind type_kind_from_str8(String8 string) {
    if (str8_compare(string, str8_lit("s32"))) return TYPE_S32;

    fprintf(stderr, "No name defined for TypeKind %.*s\n", str8_varg(string));
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

Type *make_simple_type(Arena *arena, TypeSet *type_set, TypeKind kind) {
    Type temp_type = {0};
    temp_type.kind = kind;
    return type_intern(arena, &type_set, &temp_type);
}

Type *make_function_type(Arena *arena, TypeSet *type_set, ASTNode *node) {
    Type func_type = {0};
    func_type.kind = TYPE_FUNCTION;
    
    ASTNode *param_list = node->function_decl.parameters;
    u64 param_count = param_list->parameter_list.size;
    
    func_type.params = push_array(arena, Type*, param_count);
    func_type.params_count = param_count;

    for (u64 i = 0; i < param_count; i++) {
        ASTNode *param_node = param_list->parameter_list.items[i];
        func_type.params[i] = param_node->type;
    }
    func_type.return_type = node->type;

    return type_intern(arena, &type_set, &func_type);
}

// Variable type inference: use explicit type or infer from initializer
Type *make_var_type(Arena *arena, TypeSet *type_set, ASTNode *node) {
    if (node->type) {
        return node->type;
    }
    if (node->var_decl.expr) {
        return make_expr_type(arena, type_set, node->var_decl.expr);
    }
    return make_simple_type(arena, type_set, TYPE_VOID);
}

Type *make_expr_type(Arena *arena, TypeSet *type_set, ASTNode *expr) {
    switch (expr->kind) {
        case NODE_CONST_INT: {
            return make_simple_type(arena, type_set, TYPE_S32);
        }
        default:
        break;
    }
    return make_simple_type(arena, type_set, TYPE_VOID);
}

Scope *make_child_scope(Scope* parent_scope) {
    Scope *child = push_array(parent_scope->arena, Scope, 1);
    child->arena = parent_scope->arena;
    child->parent = parent_scope;
    child->expected_return_type = parent_scope->expected_return_type;
    child->types = parent_scope->types;
    return child;
}

Type* type_resolve(String8 type_name, Scope *scope) {
    if (str8_compare(type_name, str8_lit("s32"))) return make_simple_type(scope->arena, scope->types, TYPE_S32);

    Symbol *type_symbol = table_get(scope->symbols, type_name);
    if (type_symbol && type_symbol->type) return type_symbol->type;

    fprintf(stderr, "Unknown type: %.*s\n", str8_varg(type_name));
    return (Type*){0};
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
        Symbol *symbol = table_get(scope->symbols, node->function_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of function %*.s\n", str8_varg(node->function_decl.name));
            } 
        } else {
            Type *func_type = make_function_type(scope->arena, scope->types, node);
            Symbol func_symbol = {
                .identifier = node->function_decl.name,               
                .type = func_type, 
                .is_defined = true
            };
            table_insert(scope->arena, &scope->symbols, node->function_decl.name, func_symbol);
        }
        break;
    }
    case NODE_VAR_DECL: {
        Symbol *symbol = table_get(scope->symbols, node->var_decl.name);
        if (symbol) {
            if (symbol->is_defined) {
                fprintf(stderr, "Redeclaration of variable %*.s\n", str8_varg(node->function_decl.name));
            }
        } else {
            Type *var_type = make_var_type(scope->arena, scope->types, node);
            Symbol var_symbol = {
                .identifier = node->var_decl.name,
                .type = var_type,
                .is_defined = true
            };
            table_insert(scope->arena, &scope->symbols, node->var_decl.name, var_symbol);
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
            Symbol *function_symbol = table_get(scope->symbols, node->function_decl.name);
            debug_assert(function_symbol);

            Scope *function_scope = make_child_scope(scope);

            // Resolve type from name
            String8 return_type_name = node->function_decl.return_type_name;
            Type *return_type = type_resolve(return_type_name, scope);

            node->type = return_type;
            function_scope->expected_return_type = node->type;

            // Add function parameters to function scope
            for (u64 i = 0; i < node->function_decl.parameters->parameter_list.size; i++) {
                ASTNode *parameter = node->function_decl.parameters->parameter_list.items[i];
                Symbol parameter_symbol = {
                    .identifier = parameter->parameter.name,
                    .type = parameter->type,
                    .is_defined = true,
                };
                table_insert(function_scope->arena, &function_scope->symbols, parameter->parameter.name, parameter_symbol);
            }

            type_check_node(node->function_decl.body, function_scope);
            break;
        }
        case NODE_RETURN: {
            Type *expr_type = make_expr_type(scope->arena, scope->types, node->return_val.expr);
            Type *expected_return_type = scope->expected_return_type;
            
            if (expr_type != expected_return_type) {
                fprintf(stderr, "Mismatched return type. Expected type %s, but got type %s\n", 
                    str8_from_type_kind(expected_return_type->kind).str,
                    str8_from_type_kind(expr_type->kind).str);
            }
            break;
        }
        default:
            fprintf(stderr, "Unexpected Node while type checking: %s\n", node_kind_name(node).str);
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

b32 table_insert(Arena *arena, SymbolTable **table, String8 key, Symbol value) {
    for (u64 h = str8_hash(key); *table; h <<=2) {
        if (str8_compare(key, (*table)->key)) {
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

Symbol *table_get(SymbolTable *table, String8 key) {
    for (u64 h = str8_hash(key); table; h <<= 2) {
        if (str8_compare(key, table->key)) {
            return &table->value;
        }
        table = table->child[h >> 62];
    }
    return (Symbol *){0};
}

u64 type_hash(Type *type) {
    u64 h = (u64)type->kind;

    switch (type->kind) {
        case TYPE_FUNCTION: {
            h ^= type->params_count;
            for (u64 i = 0; i < type->params_count; i++) {
                h ^= (uintptr_t)type->params[i] >> 3;
                h *= 0x9e3779b97f4a7c15;
            }
            h ^= ((uintptr_t)type->return_type >> 3);
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
        if (a->params_count != b->params_count) return false;
        if (a->return_type != b->return_type) return false;

        for (u64 i = 0; i < a->params_count; i++) {
            if (a->params[i] != b->params[i]) return false;
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
