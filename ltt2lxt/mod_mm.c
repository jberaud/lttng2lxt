/**
 * @file   mod_mm.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

static struct ltt_trace trace[2] = {{.sym = NULL}, {.sym = NULL}};

static void init_traces(void)
{
    init_trace(&trace[0], TG_MM, 1.0,  LT_SYM_F_BITS, "fault");
    init_trace(&trace[1], TG_MM, 1.01, LT_SYM_F_STRING, "fault (info)");
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
