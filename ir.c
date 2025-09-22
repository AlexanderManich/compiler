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

i32 ir_gen_expr(Expr *expr) {
    switch (expr->kind) {
    case EXPR_INT: {
        i32 reg = new_register();
        emit(IR_LOAD_CONST, reg, 0, 0, expr->int_value);
        return reg;
    }
    
    default:
        return -1; 
    }
}

void ir_gen_statement(Stmt *stmt) {
    switch (stmt->kind) {
    case STMT_RETURN: {
        i32 reg = ir_gen_expr(stmt->expr);
        emit(IR_RET, reg, 0, 0, 0);
        break;
    }
    
    default:
        break;
    }
}

void ir_gen_function(Decl *decl) {
    register_counter = 0;

    for (u64 i = 0; i < decl->func.params.size; i++) {
        // ASTNode *param = func_node->function_decl.parameters->parameter_list.items[i];
    }

    for (u64 i = 0; i < decl->func.body.size; i++) {
        ir_gen_statement(decl->func.body.items[i]);
    }
}