/**
 * @file   mod_kernel.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

struct ltt_trace *current_process = NULL;

/* nested irq stack */
static int irqtab[MAX_IRQS];
static int irqlevel = 0;

static struct ltt_trace irq_pc = {.sym = NULL};
static struct ltt_trace sirq[2] = {{.sym = NULL}, {.sym = NULL}};
static struct ltt_trace tlow[2] = {{.sym = NULL}, {.sym = NULL}};
static struct ltt_trace trace[MAX_IRQS] = {[0 ... (MAX_IRQS-1)]= {.sym = NULL}};

static void init_traces(void)
{
    init_trace(&irq_pc, TG_IRQ, 0.0, LT_SYM_F_ADDR, "IRQ (pc)");
    init_trace(&sirq[0], TG_IRQ, 100.0, LT_SYM_F_BITS, "softirq");
    init_trace(&sirq[1], TG_IRQ, 100.01, LT_SYM_F_STRING, "softirq (info)");
    init_trace(&tlow[0], TG_IRQ, 101.0, LT_SYM_F_BITS, "tasklet_low");
    init_trace(&tlow[1], TG_IRQ, 101.01, LT_SYM_F_ADDR, "tasklet_low (info)");
}

static void kernel_irq_entry_process(struct ltt_module *mod,
									 struct parse_result *res, int pass)
{
	int kernel_mode;
    unsigned int ip, irq;

	if (sscanf(res->values, " ip = %u, irq_id = %u, kernel_mode = %d",
			   &ip, &irq, &kernel_mode) != 3) {
		PARSE_ERROR(mod, res->values);
		return;
	}
    if (irq >= MAX_IRQS) {
        DIAG("invalid IRQ vector ? (%d)\n", irq);
        return;
    }

	if (pass == 1) {
        atag_store(ip);
	}
	if (pass == 2) {
        init_traces();
        init_trace(&trace[irq], TG_IRQ, 1.0+irq, LT_SYM_F_BITS, irq_tag[irq]);
        if (irqlevel >= MAX_IRQS) {
            DIAG("IRQ nesting level is too high (%d)\n", irqlevel);
            return;
        }
        emit_trace(&trace[irq], (union ltt_value)LT_S0);
        emit_trace(&irq_pc, (union ltt_value)ip);
		irqtab[irqlevel++] = irq;
	}
}
MODULE(kernel, irq_entry);

static void kernel_irq_exit_process(struct ltt_module *mod,
									struct parse_result *res, int pass)
{
	if ((pass == 2) && (irqlevel > 0)) {
		emit_trace(&trace[irqtab[--irqlevel]], (union ltt_value)LT_IDLE);
	}
}
MODULE(kernel, irq_exit);

static void kernel_softirq_entry_process(struct ltt_module *mod,
										 struct parse_result *res, int pass)
{
	int id;

	if (sscanf(res->values, " softirq_id = %d", &id) != 1) {
		PARSE_ERROR(mod, res->values);
		return;
	}
    if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
        emit_trace(&sirq[0], (union ltt_value)LT_S0);
        emit_trace(&sirq[1], (union ltt_value)"softirq %d", id);
	}
}
MODULE(kernel, softirq_entry);

static void kernel_softirq_exit_process(struct ltt_module *mod,
										struct parse_result *res, int pass)
{
    if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
		emit_trace(&sirq[0], (union ltt_value)LT_IDLE);
	}
}
MODULE(kernel, softirq_exit);

static void kernel_softirq_raise_process(struct ltt_module *mod,
										 struct parse_result *res, int pass)
{
	int id;

    if (pass == 1) {
        init_traces();
    }
	if (sscanf(res->values, " softirq_id = %d", &id) != 1) {
		PARSE_ERROR(mod, res->values);
		return;
	}
	if (pass == 2) {
		emit_trace(&sirq[0], (union ltt_value)LT_S2);
        emit_trace(&sirq[1], (union ltt_value)"raise %d", id);
	}
}
MODULE(kernel, softirq_raise);

static void kernel_tasklet_low_entry_process(struct ltt_module *mod,
											 struct parse_result *res, int pass)
{
	unsigned int data, func;

	if (sscanf(res->values, " func = 0x%x, data = %u", &func, &data) != 2) {
		PARSE_ERROR(mod, res->values);
		return;
	}
	if (pass == 1) {
        init_traces();
        atag_store(func);
	}
	if (pass == 2) {
		emit_trace(&tlow[0], (union ltt_value)LT_S0);
        emit_trace(&tlow[1], (union ltt_value)func);
	}
}
MODULE(kernel, tasklet_low_entry);

static void kernel_tasklet_low_exit_process(struct ltt_module *mod,
											struct parse_result *res, int pass)
{
	if (pass == 1) {
        init_traces();
    }
	if (pass == 2) {
		emit_trace(&tlow[0], (union ltt_value)LT_IDLE);
	}
}
MODULE(kernel, tasklet_low_exit);

static void kernel_sched_schedule_process(struct ltt_module *mod,
                                          struct parse_result *res, int pass)
{
	unsigned int prev_pid, next_pid, prev_state;

	if (sscanf(res->values, " prev_pid = %u, next_pid = %u, prev_state = %u",
               &prev_pid, &next_pid, &prev_state) != 3) {
		PARSE_ERROR(mod, res->values);
		return;
	}
    if (res->pname[0] == '\0') {
        return;
    }
    if (pass == 1) {
        find_or_add_task_trace(res->pname, next_pid);
    }
	if (pass == 2) {
        if (current_process) {
            emit_trace(current_process,(union ltt_value)LT_IDLE);
        }
        current_process = find_or_add_task_trace(res->pname, next_pid);
        emit_trace(current_process, (union ltt_value)LT_S0);
	}
}
MODULE(kernel, sched_schedule);
