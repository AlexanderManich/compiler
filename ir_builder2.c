BBlock *ir_make_bblock(IRBuilder2 *builder) {
    static u32 label = 0;

    BBlock *result = push_array(builder->arena, BBlock, 1);
    result->label = label++;
    return result;
}

void push_bblock(IRBuilder2 *builder, BBlock *block) {
    da_append(builder->arena, builder->current_func->blocks, block);
}

void push_instruction(IRBuilder2 *builder, InstructionKind kind, IROperand2 arg1, IROperand2 arg2, IROperand2 result) {
    Instruction instr = {0};
    instr.kind = kind;
    instr.arg1 = arg1;
    instr.arg2 = arg2;
    instr.result = result;
    da_append(builder->arena, builder->current_bblock->instructions, instr);
}

IROperand2 no_arg() {
    IROperand2 result = {0};
    return result;
}

IROperand2 op_symbol2(void *symbol) {
    IROperand2 result = {0};
    result.kind = OP_SYM2;
    result.symbol = symbol;
    return result;
}

IROperand2 op_temp() {
    static u32 next_temp = 0;

    IROperand2 result = {0};
    result.kind = OP_TEMP2;
    result.temp = next_temp++;
    return result;
}

IROperand2 op_label(u32 label) {
    IROperand2 result = {0};
    result.kind = OP_LABEL;
    result.label = label;
    return result;
}

IROperand2 op_const(Value value) {
    IROperand2 result = {0};
    result.kind = OP_CONST2;
    result.constant = value;
    return result;
}

IROperand2 build_expr_ir(IRBuilder2 *builder, Expr* expr) {
    IROperand2 result = {0};

    switch (expr->kind) {
        case EXPR_NAME:
            result = op_symbol2(expr->symbol); 
            break;
        case EXPR_BINARY: {
            IROperand2 lhs = build_expr_ir(builder, expr->binary.left);
            IROperand2 rhs = build_expr_ir(builder, expr->binary.right);
            
            InstructionKind instr_kind = IR2_NOOP;
            switch (expr->binary.op) {
                case TOKEN_PLUS:  instr_kind = IR2_ADD;  break;
                case TOKEN_MINUS: instr_kind = IR2_SUB;  break;
                case TOKEN_MULT:  instr_kind = IR2_MULT; break;
                case TOKEN_LT:    instr_kind = IR2_LT;   break;
                case TOKEN_GT:    instr_kind = IR2_GT;   break;
                default: debug_assert(0); break;
            }

            result = op_temp();

            push_instruction(builder, instr_kind, lhs, rhs, result);

            break;
        }
        case EXPR_CALL: {
            u32 arg_count = expr->call.params.size;
            IROperand2 *arg_ops = push_array(builder->arena, IROperand2, arg_count);
            
            for (u32 i = 0; i < arg_count; i++){
                arg_ops[i] = build_expr_ir(builder, expr->call.params.items[i]);
            }
            
            for (u32 i = arg_count; i > 0; i--) {
                push_instruction(builder, IR2_ARG, arg_ops[i-1], no_arg(), no_arg());
            }
            
            result = op_temp();
            
            IROperand2 func = op_symbol2(expr->symbol);
            
            // TODO: Maybe kinda cooked with value initialization here?
            push_instruction(builder, IR2_CALL, func, op_const((Value){.kind = VALUE_INT, .integer_value = arg_count}), result);
            
            break;
        }
        case EXPR_LITERAL:
            result = op_const(expr->tav.value);
            break;

        default: debug_assert(0); break;
    }

    return result;
}

void build_cond_ir(IRBuilder2 *builder, Expr *expr, BBlock *true_target, BBlock *false_target) {
    switch (expr->binary.op) {
        default: {
            IROperand2 cond = build_expr_ir(builder, expr);
            push_instruction(builder, IR2_JUMP_IF_FALSE, cond, op_label(false_target->label), no_arg());
            break;
        }
    }
}

void build_stmt_ir(IRBuilder2 *builder, Stmt *stmt) {
    switch (stmt->kind) {
        case STMT_DECL: {

            Decl *decl = stmt->decl;
            if (decl->kind == DECL_VAR) {
                IROperand2 rhs = build_expr_ir(builder, decl->var.expr);
                
                IROperand2 result = op_symbol2(decl->symbol);
                push_instruction(builder, IR2_WRITE, rhs, no_arg(), result);
            }
            break;
        }
        case STMT_IF: {
            b32 has_else = stmt->if_stmt.else_body.size;
            
            b32 should_gen_conds = has_else;
            // TODO: Do we need to generate all conditions if we have a single function call as an expression in a condition?
            for (u64 i = 0; i < stmt->if_stmt.if_bodies.size && !should_gen_conds; i++) {
                if (stmt->if_stmt.if_bodies.items[i].size || 
                   (stmt->if_stmt.conds.items[i]->kind == EXPR_CALL)
                ) 
                should_gen_conds = true;
            }

            BBlock *merge_block = ir_make_bblock(builder);
            
            if (should_gen_conds) {
                for (u64 i = 0; i < stmt->if_stmt.conds.size; i++) {
                    b32 last_cond = (i == stmt->if_stmt.conds.size - 1);

                    BBlock *cond_block = ir_make_bblock(builder);
                    push_bblock(builder, cond_block);

                    BBlock *else_block  = {0};
                    BBlock* next_target = {0};

                    if (!last_cond) {
                        next_target = ir_make_bblock(builder);
                        push_bblock(builder, next_target);
                    } else if (has_else) {
                        else_block  = ir_make_bblock(builder);
                        push_bblock(builder, else_block);
                        next_target = else_block;
                    } else {
                        next_target = merge_block;
                    }

                    build_cond_ir(builder, stmt->if_stmt.conds.items[i], cond_block, next_target);
                    builder->current_bblock = cond_block;

                    for (u64 j = 0; j < stmt->if_stmt.if_bodies.items[i].size; j++) {
                        build_stmt_ir(builder, stmt->if_stmt.if_bodies.items[i].items[j]);
                    }

                    push_instruction(builder, IR2_JUMP, op_label(merge_block->label), no_arg(), no_arg());

                    if (!last_cond) {
                        builder->current_bblock = next_target;
                    } else if (has_else) {
                        builder->current_bblock = else_block;
                        for (u64 j = 0; j < stmt->if_stmt.else_body.size; j++) {
                            build_stmt_ir(builder, stmt->if_stmt.else_body.items[j]);
                        }
                    } 
                }
            }

            push_bblock(builder, merge_block);
            builder->current_bblock = merge_block;

            break;
        }
        case STMT_RETURN: {
            IROperand2 value = {0};
            if (stmt->expr) {
                value = build_expr_ir(builder, stmt->expr);
            }
            push_instruction(builder, IR2_RET, value, no_arg(), no_arg());

            BBlock *next = ir_make_bblock(builder);
            push_bblock(builder, next);
            builder->current_bblock = next;

            break;
        }

        case STMT_EXPR: {
            build_expr_ir(builder, stmt->expr);

            break;
        }
        case STMT_ASSIGN: {
            IROperand2 rhs = build_expr_ir(builder, stmt->assign_stmt.rhs);
            push_instruction(builder, IR2_WRITE, rhs, no_arg(), op_symbol2(stmt->assign_stmt.lhs->symbol));

            break;
        }
        default: debug_assert(0); break;
    }
}

IRFunction2 build_function_ir(IRBuilder2 *builder, Decl *decl) {
    IRFunction2 result = {0};
    builder->current_func = &result;

    
    BBlock *bblock = ir_make_bblock(builder);
    push_bblock(builder, bblock);
    builder->current_bblock = bblock;


    for (u64 i = 0; i < decl->func.body.size; i++) {
        Stmt *stmt = decl->func.body.items[i];
        build_stmt_ir(builder, stmt);
    }

    return result;
}

IRFunctionList2 build_program_ir(IRBuilder2 *builder, DeclList program) {
    IRFunctionList2 result = {0};

    for (u64 i = 0; i < program.size; i++) {
        Decl *decl = program.items[i];
        if (decl->kind == DECL_FUNC) {
            IRFunction2 function = build_function_ir(builder, decl);
            da_append(builder->arena, result, function);
        }
    }

    return result;
}


