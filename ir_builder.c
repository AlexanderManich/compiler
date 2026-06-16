IRModule ir_make_module(Arena *arena) {
    IRModule result = {0};
    result.arena = arena;
    return result;
}

void ir_add_func_to_module(IRModule *module, IRFunction *func) {
    da_append(module->arena, module->functions, func);
}

IRFunction *ir_make_function(Arena *arena, String8 name) {
    IRFunction *result = push_array(arena, IRFunction, 1);
    result->arena = arena;
    result->name = name;
    result->next_vreg = 1; // Start first register id at 1;
    result->next_label = 1;
    result->next_stack_offset = 0;
    return result;
}

void ir_emit_func_instr(IRFunction *func, IRInstr instr) {
    if (func->blocks.size == 0) {
        LabelId label_id = ir_make_func_label(func);
        ir_emit_func_label(func, label_id);
    }

    da_append(func->arena, func->blocks.items[func->blocks.size - 1].instructions, instr);
}
LabelId ir_make_func_label(IRFunction *func) {
    return func->next_label++;
}

b32 ir_instr_push_operand(IRInstr *instr, IROperand operand) {
    if (instr->operands_count >= IR_MAX_OPERANDS) return 0;
    instr->operands[instr->operands_count++] = operand;
    return 1;
}

static inline IROperand op_reg(i32 id, u8 size) {
    IROperand result = {
        .kind = OP_REG,
        .size = size,
        .vreg = id,
    };
    return result;
}

static inline IROperand op_stack(i64 offset, u8 size) {
    IROperand result = {
        .kind = OP_STACK,
        .size = size,
        .stack_offset = offset,
    };
    return result;
}

static inline IROperand op_imm(i64 value, u8 size) {
    IROperand result = {
        .kind = OP_IMM,
        .size = size,
        .imm = value,
    };
    return result;
}

static inline IROperand op_label(LabelId label_id) {
    IROperand result = {
        .kind = OP_LABEL,
        .label_id = label_id,
    };
    return result;
}

static inline IROperand op_symbol(void *symbol) {
    IROperand result = {
        .kind = OP_SYMBOL,
        .symbol = symbol,
    };
    return result;
}

void ir_emit_func_label(IRFunction *func, LabelId label_id) {
    IRBlock block = {0};
    block.label = label_id;
    
    IRInstr instr = {0};
    instr.kind = IR_LABEL;
    ir_instr_push_operand(&instr, op_label(label_id));

    da_append(func->arena, block.instructions, instr);
    da_append(func->arena, func->blocks, block);
}
VReg ir_next_func_reg(IRFunction *func) {
    return func->next_vreg++;
}

u32 ir_alloc_stack_slot(IRFunction *func, u64 size) {
    // TODO: This probably needs to be aligned to something like 8 or 16 
    return func->next_stack_offset += size;
}

// String8 ir_instr_to_str8(Arena *arena, IRInstr *instr) {}

static inline b32 is_reg(IROperand op) {
    return (op.kind == OP_REG) && (op.vreg != 0);
}

static inline void ir_push_def(IRDefUse *du, IROperand op) {
    debug_assert(du->defs_count <= 1);
    if (is_reg(op)) du->defs[du->defs_count++] = op.vreg;
}

static inline void ir_push_use(IRDefUse *du, IROperand op) {
    debug_assert(du->uses_count <= IR_MAX_OPERANDS);
    if (is_reg(op)) du->uses[du->uses_count++] = op.vreg;
}

IRDefUse ir_collect_def_use(IRInstr *instr) {
    IRDefUse result = {0};

    u8 op_idx = 0;
    switch (instr->kind) {
        case IR_MOV: {
            ir_push_def(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_LOAD: {
            ir_push_def(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_STORE: {
            ir_push_use(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_LEA: {
            ir_push_def(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV: {
            ir_push_def(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx++]);
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_CALL: {
            // [callee, dest, param0, param1, ...]
            ir_push_use(&result, instr->operands[op_idx++]);
            ir_push_def(&result, instr->operands[op_idx++]);
            for (; op_idx < instr->operands_count; op_idx++) {
                ir_push_use(&result, instr->operands[op_idx]);
            }
            break;
        }
        case IR_RET: {
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        case IR_BR: {
            ir_push_use(&result, instr->operands[op_idx]);
            break;
        }
        default:
            break;
    }

    return result;
}

IRModule lower_program(Arena *arena, DeclList program) {
    IRModule module = ir_make_module(arena);
    for (u64 i = 0; i < program.size; i++) {
        Decl *decl = program.items[i];
        if (decl->kind == DECL_FUNC) {
            IRBuilder builder = {0};
            builder.arena = arena;
            table_init(&builder.locals_to_slots, builder.arena, 1024, table_str8_hash, table_str8_compare, table_str8_copy);
            builder.func = ir_make_function(arena, decl->name);

            lower_function(&builder, decl);
            ir_add_func_to_module(&module, builder.func);
        }
    }

    return module;
}

IRFunction *lower_function(IRBuilder *builder, Decl *func_decl) {
    IRFunction *func = builder->func;

    LabelId entry = ir_make_func_label(func);
    ir_emit_func_label(func, entry);

    for (u64 i = 0; i < func_decl->func.body.size; i++) {
        Stmt *stmt = func_decl->func.body.items[i];
        lower_stmt(builder, stmt);
    }

    IRInstr ret = {0};
    ret.kind = IR_RET;
    ret.operands_count = 0;
    ir_emit_func_instr(func, ret);

    return func;
}

VReg lower_expr(IRBuilder *builder, Expr *expr) {
    IRFunction *func = builder->func;
    switch (expr->kind) {
        case EXPR_NAME: {
            VReg reg  = ir_next_func_reg(func);
            u32 *slot = (u32 *)table_get_(&builder->locals_to_slots, &expr->name);
            if (!slot) {
                return reg;
            } else {
                IRInstr instr = {0};
                instr.kind = IR_LOAD;
                ir_instr_push_operand(&instr, op_reg(reg, expr->symbol->type->size));
                ir_instr_push_operand(&instr, op_stack(*slot, expr->symbol->type->size));
                ir_emit_func_instr(func, instr);
                return reg;
            }
        }
        case EXPR_LITERAL: {
            VReg reg = ir_next_func_reg(func);
            IRInstr instr = {0};
            instr.kind = IR_MOV;
            ir_instr_push_operand(&instr, op_reg(reg, expr->tav.type->size));
            if (expr->tav.value.kind == VALUE_INT) {
                ir_instr_push_operand(&instr, op_imm(expr->tav.value.integer_value, expr->tav.type->size));
            }
            if (expr->tav.value.kind == VALUE_STRING) {
                eprintf("YIKES");
            }
            instr.origin = expr;
            ir_emit_func_instr(func, instr);
            return reg;
        }
        case EXPR_BINARY: {
            VReg left  = lower_expr(builder, expr->binary.left);
            VReg right = lower_expr(builder, expr->binary.right);
            VReg result_reg = ir_next_func_reg(func);
            IRInstr instr = {0};
            switch (expr->binary.op) {
                case TOKEN_PLUS:  instr.kind = IR_ADD;  break;
                case TOKEN_MINUS: instr.kind = IR_SUB;  break;
                default:          instr.kind = IR_NOOP; break;
            }
            ir_instr_push_operand(&instr, op_reg(result_reg, expr->tav.type->size));
            ir_instr_push_operand(&instr, op_reg(left, expr->binary.left->tav.type->size));
            ir_instr_push_operand(&instr, op_reg(right, expr->binary.right->tav.type->size));
            instr.origin = expr;
            ir_emit_func_instr(func, instr);
            return result_reg;
        }
        case EXPR_CALL: {
            u64 param_count = expr->call.params.size; 
            VReg *param_regs = push_array(builder->arena, VReg, param_count);
            for (u64 i = 0; i < param_count; i++) {
                param_regs[i] = lower_expr(builder, expr->call.params.items[i]);
            }
            VReg result_reg = ir_next_func_reg(func);
            IRInstr instr = {0};
            instr.kind = IR_CALL;
            ir_instr_push_operand(&instr, op_symbol(&expr->call.expr->name));

            if (expr->tav.type->function.return_type == TYPE_VOID) {
                result_reg = 0;
            }
            ir_instr_push_operand(&instr, op_reg(result_reg, expr->tav.type->size)); // IDK SIZE HERE?
            for (u64 i = 0; i < param_count; i++) {
                ir_instr_push_operand(&instr, op_reg(param_regs[i], expr->call.params.items[i]->tav.type->size));
            }
            instr.origin = expr;
            ir_emit_func_instr(func, instr);
            return result_reg;
        }
        default:
            debug_assert(0 && "IRBuild not supported for EXPR_KIND");
            break;
    }
}

void lower_stmt(IRBuilder *builder, Stmt *stmt) {
    IRFunction *func = builder->func;
    switch (stmt->kind) {
        case STMT_DECL: {
            Decl *decl = stmt->decl;
            if (decl->kind == DECL_VAR) {
                u32 slot = ir_alloc_stack_slot(func, decl->var.expr->tav.type->size);
                table_add(&builder->locals_to_slots, &decl->name, &slot);
                VReg rhs_reg = lower_expr(builder, decl->var.expr);
                IRInstr instr = {0};
                instr.kind = IR_STORE;
                ir_instr_push_operand(&instr, op_stack(slot, decl->var.expr->tav.type->size));
                ir_instr_push_operand(&instr, op_reg(rhs_reg, decl->var.expr->tav.type->size));
                ir_emit_func_instr(func, instr) ;
            }
            break;
        }
        case STMT_ASSIGN: {
            Expr *left  = stmt->assign_stmt.lhs;
            Expr *right = stmt->assign_stmt.rhs;
            VReg reg_right = lower_expr(builder, right);

            u32 slot = *(u32 *)table_get_(&builder->locals_to_slots, &left->name);
            if (slot) {
                IRInstr instr = {0};
                instr.kind = IR_STORE;
                ir_instr_push_operand(&instr, op_stack(slot, right->tav.type->size));
                ir_instr_push_operand(&instr, op_reg(reg_right, right->tav.type->size));
                ir_emit_func_instr(func, instr);
            }
            break;
        }
        case STMT_RETURN: {
            VReg reg = 0;
            if (stmt->expr) {
                reg = lower_expr(builder, stmt->expr);
            }
            IRInstr instr = {0};
            instr.kind = IR_RET;
            if (reg) {
                instr.operands_count = 1;
                ir_instr_push_operand(&instr, op_reg(reg, 8));
            }  
            ir_emit_func_instr(func, instr);
            break;
        }
        case STMT_EXPR: {
            VReg reg = lower_expr(builder, stmt->expr);
            break;
        }
        case STMT_BLOCK: {
            for (u32 i = 0; i < stmt->block.size; i++) {
                lower_stmt(builder, stmt->block.items[i]);
            }
            break;
        }
        default:
            debug_assert(0 && "IRBuild not supported for STMT_KIND");
            break;
    }
}