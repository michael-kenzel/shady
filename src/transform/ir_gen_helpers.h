#ifndef SHADY_IR_GEN_HELPERS_H
#define SHADY_IR_GEN_HELPERS_H

#include "shady/ir.h"

typedef struct Instructions_ {
    IrArena* arena;
    struct List* list;
} Instructions;

Instructions begin_instructions(IrArena*);
void copy_instructions(Instructions, Nodes);
void append_instr(Instructions, const Node*);
Nodes finish_instructions(Instructions);

const Node* wrap_in_let(IrArena*, const Node*);

void gen_push_value_stack(Instructions instructions, const Node* value);
void gen_push_values_stack(Instructions instructions, Nodes values);
void gen_push_fn_stack(Instructions instructions, const Node* fn_ptr);
const Node* gen_pop_fn_stack(Instructions instructions, String var_name);
const Node* gen_pop_value_stack(Instructions instructions, String var_name, const Type* type);
Nodes gen_pop_values_stack(Instructions instructions, String var_name, const Nodes types);

Nodes gen_primop(Instructions, PrimOp);
const Node* gen_load(Instructions, const Node* ptr);
void gen_store(Instructions, const Node* ptr, const Node* value);
const Node* gen_lea(Instructions, const Node* base, const Node* offset, Nodes selectors);

#endif
