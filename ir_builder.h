typedef u32 LabelId;
typedef u32 VReg;

typedef enum IROperandKind {
    OP_REG,
    OP_STACK,
    OP_IMM,
    OP_LABEL,
    OP_SYMBOL,
} IROperandKind;

typedef struct IROperand {
    IROperandKind kind;
    u8 size;
    union {
        VReg vreg;
        i64 imm;
        u64 stack_offset;
        void *symbol;
        LabelId label_id;
    };
} IROperand;

typedef enum IRInstrKind {
    IR_NOOP,
    IR_LABEL,
    IR_MOV,
    IR_LOAD,
    IR_STORE,
    IR_LEA,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_CALL,
    IR_RET,
    IR_JMP,
    IR_BR,
    IR_ALLOC_STACK
} IRInstrKind;

#define IR_MAX_OPERANDS 16

typedef struct IRInstr {
    IRInstrKind kind;
    u32 global_id;
    IROperand operands[IR_MAX_OPERANDS];
    u64 operands_count;
    Expr *origin;
} IRInstr;

typedef struct IRInstrList {
    IRInstr *items;
    u64 size;
    u64 capacity;
} InstrList;

typedef struct IRBlock {
    LabelId label;
    InstrList instructions;
} IRBlock;

typedef struct IRBlockList {
    IRBlock *items;
    u64 size;
    u64 capacity;
} IRBlockList;

typedef struct IRFunction {
    String8 name;
    IRBlockList blocks;

    Arena *arena;

    u32 next_vreg;
    u32 next_label;
    u32 next_stack_offset;
} IRFunction;

typedef struct IRFunctionList {
    IRFunction **items;
    u64 size;
    u64 capacity;
} IRFunctionList;

typedef struct IRModule {
    IRFunctionList functions;
    Arena *arena;
} IRModule;

IRModule ir_make_module(Arena *arena);
void ir_add_func_to_module(IRModule *module, IRFunction *func);
IRFunction *ir_make_function(Arena *arena, String8 name);
void ir_emit_func_instr(IRFunction *func, IRInstr instr);
LabelId ir_make_func_label(IRFunction *func);
void ir_emit_func_label(IRFunction *func, LabelId label_id);
VReg ir_next_func_reg(IRFunction *func);
u32 ir_alloc_stack_slot(IRFunction *func, u64 size);
b32 ir_instr_push_operand(IRInstr *instr, IROperand operand);
String8 ir_instr_to_str8(Arena *arena, IRInstr *instr);

static inline IROperand op_reg(i32 id, u8 size);
static inline IROperand op_stack(i64 offset, u8 size);
static inline IROperand op_imm(i64 value, u8 size);
static inline IROperand op_label(LabelId label_id);
static inline IROperand op_symbol(void *symbol);

typedef struct IRBuilder {
    Arena *arena;
    IRFunction *func; // Current function being built;
    Table locals_to_slots;
} IRBuilder;

IRModule lower_program(Arena *arena, DeclList program);
IRFunction *lower_function(IRBuilder *builder, Decl *func_decl);
VReg lower_expr(IRBuilder *builder, Expr *expr);
void lower_stmt(IRBuilder *builder, Stmt *stmt);

typedef struct IRDefUse {
    VReg defs[1];
    u8 defs_count;

    VReg uses[IR_MAX_OPERANDS];
    u8 uses_count;
} IRDefUse;

IRDefUse ir_collect_def_use(IRInstr *instr);

void reg_alloc_function(IRFunction *func);