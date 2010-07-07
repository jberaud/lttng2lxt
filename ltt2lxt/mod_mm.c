/**
 * @file   mod_mm.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

static struct ltt_trace trace[2] = {{.sym = NULL}, {.sym = NULL}};
static struct ltt_trace switchmm_trace;

static void init_traces(void)
{
    init_trace(&trace[0], TG_MM, 1.0,  LT_SYM_F_BITS, "fault");
    init_trace(&trace[1], TG_MM, 1.01, LT_SYM_F_STRING, "fault (info)");
    init_trace(&switchmm_trace, TG_MM, 1.02, LT_SYM_F_BITS, "switch");
}

static void mm_handle_fault_entry_process(struct ltt_module *mod,
                                          struct parse_result *res, int pass)
{
	uint32_t ip;
	int wr;
	unsigned long long address;

    if (pass == 1) {
        init_traces();
    }
	if (sscanf(res->values, " address = %llu, ip = 0x%x, write_access = %d",
			   &address, &ip, &wr) != 3) {
		PARSE_ERROR(mod, res->values);
		return;
	}
	if (pass == 2) {
		emit_trace(&trace[0], (union ltt_value)(wr?LT_S2:LT_S0));
		emit_trace(&trace[1], (union ltt_value)"%c@0x%08x",
                   (wr)? 'W' : 'R', (uint32_t)address);
	}
}
MODULE(mm, handle_fault_entry);

static void mm_handle_fault_exit_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
		emit_trace(&trace[0], (union ltt_value)LT_IDLE);
	}
}
MODULE(mm, handle_fault_exit);

static void mm_switch_mm_enter_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
		emit_trace(&switchmm_trace, (union ltt_value)LT_IDLE);
	}
}
MODULE(mm, switch_mm_enter);

static void mm_switch_mm_exit_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
		emit_trace(&switchmm_trace, (union ltt_value)LT_S0);
	}
}
MODULE(mm, switch_mm_exit);
