#include "emit_c.h"

#include "portability.h"
#include "log.h"
#include "dict.h"

#include "../../type.h"
#include "../../ir_private.h"

#include <assert.h>
#include <stdlib.h>

#pragma GCC diagnostic error "-Wswitch"

void emit_pack_code(Printer* p, Strings src, String dst) {
    for (size_t i = 0; i < src.count; i++) {
        print(p, "\n%s->_%d = %s", dst, src.strings[i], i);
    }
}

void emit_unpack_code(Printer* p, String src, Strings dst) {
    for (size_t i = 0; i < dst.count; i++) {
        print(p, "\n%s = %s->_%d", dst.strings[i], src, i);
    }
}

static Strings emit_variable_declarations(Emitter* emitter, Printer* p, String given_name, Strings* given_names, Nodes types) {
    if (given_names)
        assert(given_names->count == types.count);
    LARRAY(String, names, types.count);
    for (size_t i = 0; i < types.count; i++) {
        VarId id = fresh_id(emitter->arena);
        String name = given_names ? given_names->strings[i] : given_name;
        assert(name);
        names[i] = format_string(emitter->arena, "%s_%d", name, id);
        print(p, "\n%s;", c_emit_type(emitter, types.nodes[i], names[i]));
    }
    return strings(emitter->arena, types.count, names);
}

static void emit_primop(Emitter* emitter, Printer* p, const Node* node, InstructionOutputs outputs) {
    assert(node->tag == PrimOp_TAG);
    const PrimOp* prim_op = &node->payload.prim_op;
    enum {
        Infix, Prefix
    } m = Infix;
    String s = NULL;
    CValue rhs = NULL;
    switch (prim_op->op) {
        case unit_op:
            return;
        case quote_op: {
            rhs = to_cvalue(emitter, emit_value(emitter, prim_op->operands.nodes[0]));
            break;
        }
        case add_op: s = "+";  break;
        case sub_op: s = "-";  break;
        case mul_op: s = "*";  break;
        case div_op: s = "/";  break;
        case mod_op: s = "%";  break;
        case neg_op: s = "-"; m = Prefix; break;
        case gt_op:  s = ">";  break;
        case gte_op: s = ">="; break;
        case lt_op:  s = "<";  break;
        case lte_op: s = "<="; break;
        case eq_op:  s = "=="; break;
        case neq_op: s = "!="; break;
        case and_op: s = "&";  break;
        case or_op:  s = "|";  break;
        case xor_op: s = "^";  break;
        case not_op: s = "!"; m = Prefix; break;
        case rshift_logical_op:break;
        case rshift_arithm_op:break;
        case lshift_op:break;
        case assign_op:break;
        case subscript_op:break;
        case alloca_op:break;
        case alloca_slot_op:break;
        case alloca_logical_op:break;
        case load_op: {
            CAddr dereferenced = deref_term(emitter, emit_value(emitter, first(prim_op->operands)));
            outputs.results[0] = term_from_cvalue(dereferenced);
            outputs.needs_binding[0] = true;
            return;
        }
        case store_op: {
            CAddr dereferenced = deref_term(emitter, emit_value(emitter, first(prim_op->operands)));
            CValue cvalue = to_cvalue(emitter, emit_value(emitter, prim_op->operands.nodes[1]));
            print(p, "\n%s = %s;", dereferenced, cvalue);
            return;
        } case lea_op: {
            CTerm acc = emit_value(emitter, prim_op->operands.nodes[0]);

            const IntLiteral* offset_static_value = resolve_to_literal(prim_op->operands.nodes[1]);
            if (!offset_static_value || offset_static_value->value.i64 != 0) {
                CTerm offset = emit_value(emitter, prim_op->operands.nodes[1]);
                acc = term_from_cvar(format_string(emitter->arena, "(%s[%s])", deref_term(emitter, acc), to_cvalue(emitter, offset)));
            }

            const Type* t = extract_operand_type(prim_op->operands.nodes[0]->type);
            assert(t->tag == PtrType_TAG);
            t = t->payload.ptr_type.pointed_type;
            for (size_t i = 2; i < prim_op->operands.count; i++) {
                switch (is_type(t)) {
                    case ArrType_TAG: {
                        CTerm index = emit_value(emitter, prim_op->operands.nodes[i]);
                        acc = term_from_cvar(format_string(emitter->arena, "(%s[%s])", deref_term(emitter, acc), to_cvalue(emitter, index)));
                        break;
                    }
                    case RecordType_TAG: error("TODO");
                    default: error("lea can't work on this");
                }
            }
            assert(outputs.count == 1);
            outputs.results[0] = acc;
            outputs.needs_binding[0] = false;
            return;
        }
        case make_op: break;
        case select_op:break;
        case convert_op:break;
        case reinterpret_op: {
            assert(outputs.count == 1);
            CTerm src = emit_value(emitter, first(prim_op->operands));
            const Type* src_type = extract_operand_type(first(prim_op->operands)->type);
            const Type* dst_type = first(prim_op->type_arguments);
            if (src_type->tag == PtrType_TAG && dst_type->tag == PtrType_TAG || true) {
                CType t = emit_type(emitter, prim_op->type_arguments.nodes[0], NULL);
                outputs.results[0] = term_from_cvalue(format_string(emitter->arena, "(%s) %s", t, to_cvalue(emitter, src)));
                outputs.needs_binding[0] = false;
            } else {
                assert(false);
            }
            return;
        }
        case extract_op:break;
        case extract_dynamic_op:break;
        case push_stack_op:break;
        case pop_stack_op:break;
        case push_stack_uniform_op:break;
        case pop_stack_uniform_op:break;
        case get_stack_pointer_op:break;
        case get_stack_pointer_uniform_op:break;
        case set_stack_pointer_op:break;
        case set_stack_pointer_uniform_op:break;
        case subgroup_elect_first_op:break;
        case subgroup_broadcast_first_op:break;
        case subgroup_active_mask_op:break;
        case subgroup_ballot_op:break;
        case subgroup_local_id_op:break;
        case empty_mask_op:break;
        case mask_is_thread_active_op:break;
        case debug_printf_op:break;
        case PRIMOPS_COUNT: assert(false); break;
    }

    assert(outputs.count == 1);
    outputs.needs_binding[0] = true;
    if (s == NULL) {
        if (rhs)
            outputs.results[0] = term_from_cvalue(rhs);
        else
            outputs.results[0] = term_from_cvalue(format_string(emitter->arena, "/* todo: implement %s */", primop_names[prim_op->op]));
        return;
    }

    switch (m) {
        case Infix: {
            CTerm a = emit_value(emitter, prim_op->operands.nodes[0]);
            CTerm b = emit_value(emitter, prim_op->operands.nodes[1]);
            outputs.results[0] = term_from_cvalue(
                    format_string(emitter->arena, "%s %s %s", to_cvalue(emitter, a), s, to_cvalue(emitter, b)));
            break;
        }
        case Prefix: {
            CTerm operand = emit_value(emitter, prim_op->operands.nodes[0]);
            outputs.results[0] = term_from_cvalue(format_string(emitter->arena, "%s%s", s, to_cvalue(emitter, operand)));
            break;
        } default: assert(false);
    }
}

static void emit_call(Emitter* emitter, Printer* p, const Node* call, InstructionOutputs outputs) {
    Nodes args;
    if (call->tag == LeafCall_TAG)
        args = call->payload.leaf_call.args;
    else if (call->tag == IndirectCall_TAG)
        args = call->payload.indirect_call.args;
    else
        assert(false);

    Growy* g = new_growy();
    Printer* paramsp = open_growy_as_printer(g);
    for (size_t i = 0; i < args.count; i++) {
        print(paramsp, to_cvalue(emitter, emit_value(emitter, args.nodes[i])));
        if (i + 1 < args.count)
            print(paramsp, ", ");
    }

    CValue callee;
    if (call->tag == LeafCall_TAG) {
        emit_decl(emitter, call->payload.leaf_call.callee);
        callee = to_cvalue(emitter, *lookup_existing_term(emitter, call->payload.leaf_call.callee));
    } else
        callee = to_cvalue(emitter, emit_value(emitter, call->payload.indirect_call.callee));

    String params = printer_growy_unwrap(paramsp);

    Nodes yield_types = unwrap_multiple_yield_types(emitter->arena, call->type);
    assert(yield_types.count == outputs.count);
    if (yield_types.count > 1) {
        String named = unique_name(emitter->arena, "result");
        print(p, "\n%s = %s(%s);", emit_type(emitter, call->type, named), callee, params);
        for (size_t i = 0; i < yield_types.count; i++) {
            outputs.results[i] = term_from_cvalue(format_string(emitter->arena, "%s->_%d", named, i));
            outputs.needs_binding[i] = false;
        }
    } else if (yield_types.count == 1) {
        outputs.results[0] = term_from_cvalue(format_string(emitter->arena, "%s(%s)", callee, params));
        outputs.needs_binding[0] = true;
    } else {
        print(p, "\n%s(%s);", callee, params);
    }
    free_tmp_str(params);
}

static const Node* get_anonymous_lambda_body(const Node* lambda) {
    assert(is_anonymous_lambda(lambda));
    return lambda->payload.anon_lam.body;
}

static Nodes get_anonymous_lambda_params(const Node* lambda) {
    assert(is_anonymous_lambda(lambda));
    return lambda->payload.anon_lam.params;
}

static void emit_if(Emitter* emitter, Printer* p, const Node* if_instr, InstructionOutputs outputs) {
    assert(if_instr->tag == If_TAG);
    const If* if_ = &if_instr->payload.if_instr;
    Emitter sub_emiter = *emitter;
    Strings ephis = emit_variable_declarations(emitter, p, "loop_break_phi", NULL, if_->yield_types);
    sub_emiter.phis.selection = ephis;

    assert(get_anonymous_lambda_params(if_->if_true).count == 0);
    String true_body = emit_lambda_body(&sub_emiter, get_anonymous_lambda_body(if_->if_true), NULL);
    CValue condition = to_cvalue(emitter, emit_value(emitter, if_->condition));
    print(p, "\nif (%s) %s", condition, true_body);
    free_tmp_str(true_body);
    if (if_->if_false) {
        assert(get_anonymous_lambda_params(if_->if_false).count == 0);
        String false_body = emit_lambda_body(&sub_emiter, get_anonymous_lambda_body(if_->if_false), NULL);
        print(p, " else %s", false_body);
        free_tmp_str(false_body);
    }

    assert(outputs.count == ephis.count);
    for (size_t i = 0; i < outputs.count; i++) {
        outputs.results[i] = term_from_cvalue(ephis.strings[i]);
        outputs.needs_binding[i] = false;
    }
}

static void emit_match(Emitter* emitter, Printer* p, const Node* match_instr, InstructionOutputs outputs) {
    assert(match_instr->tag == Match_TAG);
    const Match* match = &match_instr->payload.match_instr;
    Emitter sub_emiter = *emitter;
    Strings ephis = emit_variable_declarations(emitter, p, "loop_break_phi", NULL, match->yield_types);
    sub_emiter.phis.selection = ephis;

    CValue inspectee = to_cvalue(emitter, emit_value(emitter, match->inspect));
    print(p, "\nswitch (%s) {", inspectee);
    indent(p);
    for (size_t i = 0; i < match->cases.count; i++) {
        String case_body = emit_lambda_body(&sub_emiter, get_anonymous_lambda_body(match->cases.nodes[i]), NULL);
        CValue literal = to_cvalue(emitter, emit_value(emitter, match->literals.nodes[i]));
        print(p, "\ncase %s: %s\n", literal, case_body);
        free_tmp_str(case_body);
    }
    if (match->default_case) {
        String default_case_body = emit_lambda_body(&sub_emiter, get_anonymous_lambda_body(match->default_case), NULL);
        print(p, "\ndefault: %s\n", default_case_body);
        free_tmp_str(default_case_body);
    }
    deindent(p);
    print(p, "\n}");

    assert(outputs.count == ephis.count);
    for (size_t i = 0; i < outputs.count; i++) {
        outputs.results[i] = term_from_cvalue(ephis.strings[i]);
        outputs.needs_binding[i] = false;
    }
}

static void emit_loop(Emitter* emitter, Printer* p, const Node* loop_instr, InstructionOutputs outputs) {
    assert(loop_instr->tag == Loop_TAG);
    const Loop* loop = &loop_instr->payload.loop_instr;

    Emitter sub_emiter = *emitter;
    Nodes params = get_anonymous_lambda_params(loop->body);
    Strings param_names = extract_variable_names(emitter->arena, params);
    Strings eparams = emit_variable_declarations(emitter, p, NULL, &param_names, extract_variable_types(emitter->arena, params));
    for (size_t i = 0; i < params.count; i++)
        register_emitted(&sub_emiter, params.nodes[i], term_from_cvalue(eparams.strings[i]));

    sub_emiter.phis.loop_continue = eparams;
    Strings ephis = emit_variable_declarations(emitter, p, "loop_break_phi", NULL, loop->yield_types);
    sub_emiter.phis.loop_break = ephis;

    String body = emit_lambda_body(&sub_emiter, get_anonymous_lambda_body(loop->body), NULL);
    print(p, "\nwhile(true) %s", body);
    free_tmp_str(body);

    assert(outputs.count == ephis.count);
    for (size_t i = 0; i < outputs.count; i++) {
        outputs.results[i] = term_from_cvalue(ephis.strings[i]);
        outputs.needs_binding[i] = false;
    }
}

void emit_instruction(Emitter* emitter, Printer* p, const Node* instruction, InstructionOutputs outputs) {
    assert(is_instruction(instruction));

    switch (is_instruction(instruction)) {
        case NotAnInstruction: assert(false);
        case Instruction_PrimOp_TAG:       emit_primop(emitter, p, instruction, outputs); break;
        case Instruction_LeafCall_TAG:
        case Instruction_IndirectCall_TAG: emit_call  (emitter, p, instruction, outputs); break;
        case Instruction_If_TAG:           emit_if    (emitter, p, instruction, outputs); break;
        case Instruction_Match_TAG:        emit_match (emitter, p, instruction, outputs); break;
        case Instruction_Loop_TAG:         emit_loop  (emitter, p, instruction, outputs); break;
        case Instruction_Control_TAG: error("TODO")
    }
}
