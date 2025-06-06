#include "ir.h"
#include "parser.h"

Instruction ir_code[IR_MAX];
i32 ir_count = 0;

void emit(Op op, i32 dest, i32 src1, i32 src2, i32 imm) {
    if (ir_count < IR_MAX) {
        ir_code[ir_count++] = (Instruction){
            .op = op,
            .src1 = src1,
            .src2 = src2,
            .dest = dest,
            .imm = imm
        };
    }
}

i32 register_counter = 0;

i32 new_register() {
    return register_counter++;
}

i32 ir_gen_expr(ASTNode *node) {
    switch (node->kind) {
    case NODE_CONST_INT: {
        i32 reg = new_register();
        emit(IR_LOAD_CONST, reg, 0, 0, node->const_int.value);
        return reg;
    }
    
    default:
        return -1; 
    }
}

void ir_gen_statement(ASTNode *node) {
    switch (node->kind) {
    case NODE_RETURN: {
        i32 reg = ir_gen_expr(node->return_val.expr);
        emit(IR_RET, reg, 0, 0, 0);
        break;
    }
    
    default:
        break;
    }
}

void ir_gen_function(ASTNode *func_node) {
    register_counter = 0;

    for (u64 i = 0; i < func_node->function_decl.parameters->parameter_list.size; i++) {
        ASTNode *param = func_node->function_decl.parameters->parameter_list.items[i];
    }

    for (u64 i = 0; i < func_node->function_decl.body->block.size; i++) {
        ir_gen_statement(func_node->function_decl.body->block.items[i]);
    }
}