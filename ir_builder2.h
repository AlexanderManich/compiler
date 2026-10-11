typedef enum IROperandKind2 {
    OP_INVALID,
    OP_SYM2,
    OP_CONST2,
    OP_TEMP2,
    OP_LABEL,
} IROperandKind2;

typedef struct IROperand2 {
    IROperandKind2 kind;

    union {
        void *symbol;
        Value constant;
        u32 temp;
        u32 label;
    };
} IROperand2;

typedef enum InstructionKind {
    IR2_NOOP,
    IR2_ADD,
    IR2_SUB,
    IR2_MULT,
    IR2_WRITE,
    IR2_READ,
    IR2_ARG,
    IR2_CALL,
    IR2_RET,
    IR2_JUMP_IF_FALSE,
    IR2_JUMP,
    IR2_LT,
    IR2_GT,
} InstructionKind;

typedef struct Instruction {
    InstructionKind kind;
    IROperand2 arg1;
    IROperand2 arg2;
    IROperand2 result;
} Instruction;

typedef struct InstructionList {
    Instruction *items;
    u64 size;
    u64 capacity;
} InstructionList;

typedef struct BBlock {
    u32 label;
    InstructionList instructions;
} BBlock;

typedef struct BBlockList {
    BBlock **items;
    u64 size;
    u64 capacity;
} BBlockList;

typedef struct IRFunction2 {
    String8 name;
    BBlockList blocks;
} IRFunction2;

typedef struct IRFunctionList2 {
    IRFunction2 *items;
    u64 size;
    u64 capacity;
} IRFunctionList2;

typedef struct IRBuilder2 {
    Arena *arena;
    IRFunction2 *current_func;
    BBlock      *current_bblock;
} IRBuilder2;

IRFunctionList2 build_program_ir(IRBuilder2 *builder, DeclList program);
IRFunction2 build_function_ir(IRBuilder2 *builder, Decl *decl);
void build_stmt_ir(IRBuilder2 *builder, Stmt *stmt);
IROperand2 build_expr_ir(IRBuilder2 *builder, Expr* expr);

IROperand2 op_const(Value value);
IROperand2 op_temp();
IROperand2 op_symbol2(void *symbol);

void push_instruction(IRBuilder2 *builder, InstructionKind kind, IROperand2 arg1, IROperand2 arg2, IROperand2 result);

void ir_start_bblock(IRBuilder2 *builder, IRFunction2 *func);