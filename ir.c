#include "ir.h"
#include "type_check.h"
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
        eprintf("Tried to generate IR for unimplemented expression kind %.*s", str8_varg(expr_kind_name(expr->kind)));
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
    case STMT_DECL: {
        Decl *decl = stmt->decl;
        i32 reg = ir_gen_expr(decl->var.expr);
        break;
    }
    
    default:
        eprintf("Tried to generate IR for unimplemented statement kind %.*s", str8_varg(stmt_kind_name(stmt->kind)));
        break;
    }
}

void ir_gen_function(Decl *decl) {
    register_counter = 0;

    for (u64 i = 0; i < decl->func.params.size; i++) {
        // FuncParam *param = decl->func.params.items[i];
    }

    for (u64 i = 0; i < decl->func.body.size; i++) {
        ir_gen_statement(decl->func.body.items[i]);
    }
}