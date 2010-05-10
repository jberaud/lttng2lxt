/**
 * @file   mod_kernel.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"
/* generated from strace with 
   grep "{"  syscallent.h | cut -d\" -f2 | sed -e 's/^/"/1' -e 's/$/",/g' > /tmp/syscall.h
*/
static const char *syscall_name[] = {
#include "syscall.h"
};

#define PROCESS_IDLE LT_IDLE
#define PROCESS_KERNEL (gtkwave_parrot?LT_S0:LT_1)
#define PROCESS_USER (gtkwave_parrot?LT_S1:LT_S0)
#define PROCESS_WAKEUP (gtkwave_parrot?LT_S2:LT_0)
#define PROCESS_DEAD LT_0

#define SOFTIRQ_IDLE LT_IDLE
#define SOFTIRQ_RUNNING LT_S0
#define SOFTIRQ_RAISING (gtkwave_parrot?LT_S2:LT_0)

#define IRQ_IDLE LT_IDLE
#define IRQ_RUNNING LT_S0
#define IRQ_PREEMPT (gtkwave_parrot?LT_S2:LT_0)

#define IDLE_CPU_IDLE IRQ_IDLE
#define IDLE_CPU_RUNNING IRQ_RUNNING
#define IDLE_CPU_PREEMPT IRQ_IDLE

/* nested irq stack */
static int irqtab[MAX_IRQS];
static int irqlevel = 0;

/* nested softirq state */
enum {
    SOFTIRQS_IDLE,
    SOFTIRQS_RAISE,
    SOFTIRQS_RUN,
};
/* sofirq val */
enum
{
    HI_SOFTIRQ=0,
    TIMER_SOFTIRQ,
    NET_TX_SOFTIRQ,
    NET_RX_SOFTIRQ,
    BLOCK_SOFTIRQ,
    TASKLET_SOFTIRQ,
    SCHED_SOFTIRQ,
//#ifdef CONFIG_HIGH_RES_TIMERS
    HRTIMER_SOFTIRQ,
//#endif
    RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */
};

#define str(x) [x] = #x
static char *sofirq_tag [] = {
    str(HI_SOFTIRQ),
    str(TIMER_SOFTIRQ),
    str(NET_TX_SOFTIRQ),
    str(NET_RX_SOFTIRQ),
    str(BLOCK_SOFTIRQ),
    str(TASKLET_SOFTIRQ),
    str(SCHED_SOFTIRQ),
    str(HRTIMER_SOFTIRQ),
    str(RCU_SOFTIRQ),
};
#undef str
static int softirqstate;
static double softirqtime;
static char softirqtask[30];
static double irqtime;
static double timer3clock;
static double timer3diff;
enum {
    IDLE_IDLE,
    IDLE_RUNNING,
};
static int idle_cpu_state;

static struct ltt_trace irq_pc;
static struct ltt_trace sirq[3];
static struct ltt_trace tlow[2];
static struct ltt_trace trace[MAX_IRQS];
static struct ltt_trace syscall_id;
static struct ltt_trace syscall_pc;
static struct ltt_trace sched_event;
static struct ltt_trace mode;
static struct ltt_trace printk_pc;
static struct ltt_trace jiffies;
static struct ltt_trace parrot_evt;
static struct ltt_trace idle_cpu;
static struct ltt_trace cpu_load;

static void init_traces(void)
{
    init_trace(&irq_pc, TG_IRQ, 0.0, LT_SYM_F_ADDR, "IRQ (pc)");
    init_trace(&sirq[0], TG_IRQ, 100.0, LT_SYM_F_BITS, "softirq");
    init_trace(&sirq[1], TG_IRQ, 100.01, LT_SYM_F_STRING, "softirq (info)");
    init_trace(&sirq[2], TG_IRQ, 100.02, LT_SYM_F_STRING, "softirq (info2)");
    init_trace(&tlow[0], TG_IRQ, 101.0, LT_SYM_F_BITS, "tasklet_low");
    init_trace(&tlow[1], TG_IRQ, 101.01, LT_SYM_F_ADDR, "tasklet_low (info)");
    init_trace(&syscall_id, TG_PROCESS, 0.0, LT_SYM_F_U16, "SYSCALL");
    init_trace(&syscall_pc, TG_PROCESS, 0.0, LT_SYM_F_ADDR, "SYSCALL (pc)");
    init_trace(&sched_event, TG_PROCESS, 0.0, LT_SYM_F_STRING, "Sched event");
    init_trace(&mode, TG_PROCESS, 0.0, LT_SYM_F_STRING, "MODE");
    init_trace(&printk_pc, TG_PROCESS, 0.0, LT_SYM_F_ADDR, "printk (pc)");
    init_trace(&jiffies, TG_IRQ, 0.0, LT_SYM_F_INTEGER, "jiffies");
    init_trace(&parrot_evt, TG_PROCESS, 0.0, LT_SYM_F_STRING, "kernel event");
    init_trace(&idle_cpu, TG_PROCESS, 0.0, LT_SYM_F_BITS, "global idle");
    init_trace(&cpu_load, TG_IRQ, 0.0, LT_SYM_F_ANALOG, "cpu load");
}

static double emit_cpu_idle_state(struct parse_result *res, union ltt_value val)
{
    static double run_start;
    static double total_run;
    double ret;
    if (val.state) {
        emit_trace(&idle_cpu, val);

        //printf("clock %f %s start %f  total %f\n", res->clock, val.state, run_start, total_run);
		/* if running account the time */
        if (strcmp(val.state, IRQ_RUNNING) == 0) {
			/* if already started, do nothing */
            if (run_start == 0)
                run_start = res->clock;
        }
		/* if stoping record the idle time */
        else {
			/* if already stop, do nothing */
            if (run_start > 0) {
                total_run += res->clock - run_start;
                run_start = 0;
            }
        }
        //printf("%s start %f  total %f\n", val.state, run_start, total_run);
        ret = total_run;
    }
	/* give the total idle time and reset counter */
    else {
        /* idle is running */
        if (run_start > 0) {
            total_run += res->clock - run_start;
            run_start = res->clock;
        }
        ret = total_run;
        /* reset counter */
        total_run = 0;
        //printf("total clock : %f ret : %f\n", res->clock, ret);
    }
    return ret;
}

static void kernel_common(struct parse_result *res, int pass)
{
    static char old_mode [15];
    static double start_time;

    if (pass == 1) {
        init_traces();
        find_or_add_task_trace(res->pname, res->pid);
    }
    if (pass == 2) {
        if (strcmp(old_mode, res->mode))
            emit_trace(&mode, (union ltt_value)res->mode);
        memcpy(old_mode, res->mode, sizeof(old_mode));
        old_mode[sizeof(old_mode)-1] = 0;

		/* 300 ms average for cpu load */
		if (res->clock - start_time > 0.3) {
			double idle_run = emit_cpu_idle_state(res, (union ltt_value)(char *)NULL);
			if (start_time > 0) {
				emit_trace(&cpu_load, (union ltt_value)(1.0-idle_run/(res->clock - start_time)));
			}
			start_time = res->clock;
		}
    }
}

static void kernel_irq_entry_process(struct ltt_module *mod,
                                     struct parse_result *res, int pass)
{
    int kernel_mode;
    unsigned int ip, irq;

    kernel_common(res, pass);
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
        init_trace(&trace[irq], TG_IRQ, 1.0+irq, LT_SYM_F_BITS, irq_tag[irq]);
        atag_store(ip);
    }
    if (pass == 2) {
        if (irqlevel >= MAX_IRQS) {
            DIAG("IRQ nesting level is too high (%d)\n", irqlevel);
            return;
        }
        if (irqlevel > 0 && irq == irqtab[irqlevel-1]) {
            DIAG("IRQ reentering in same irq (broken trace ?) %d\n", irqlevel);
            return;
        }

        if (irqlevel > 0) {
            TDIAG(res, "nesting irq %s -> %s\n", irq_tag[irqtab[irqlevel-1]],
                    irq_tag[irq]);
            emit_trace(&trace[irqtab[irqlevel-1]], (union ltt_value)IRQ_PREEMPT);
        }
        emit_trace(&trace[irq], (union ltt_value)IRQ_RUNNING);
        emit_trace(&irq_pc, (union ltt_value)ip);
        irqtab[irqlevel++] = irq;

        if (irqlevel == 1)
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_PREEMPT);
        /* stat stuff */
        if (irq == 19) {
            if (timer3clock > 0) {
                double diff = res->clock - timer3clock;
                if (timer3diff > 0) {
                    /* we allow a jitter of 0,1 ms */
                    if (diff > timer3diff + 0.0001) {
                        TDIAG(res, "late irq !!! system timer took %fs (instead of %fs)\n", diff, timer3diff);
                    }
                }
                else
                    timer3diff = diff;
            }

            timer3clock = res->clock;
        }
        /* only account on the first irq */
        if (irqlevel == 1)
            irqtime = res->clock;
        /* end stat stuff */
    }
}
MODULE(kernel, irq_entry);

static void kernel_irq_exit_process(struct ltt_module *mod,
                                    struct parse_result *res, int pass)
{
    kernel_common(res, pass);
    if ((pass == 2) && (irqlevel > 0)) {
        emit_trace(&trace[irqtab[--irqlevel]], (union ltt_value)IRQ_IDLE);
        if (irqlevel > 0) {
            emit_trace(&trace[irqtab[irqlevel-1]], (union ltt_value)IRQ_RUNNING);
        }
        if (irqlevel == 0 && idle_cpu_state == IDLE_RUNNING)
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_RUNNING);
        else 
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_IDLE);
        /* stat stuff */
        /* we allow up to 0.1ms irq */
        if (irqlevel == 0 && res->clock - irqtime > 0.0001)
            TDIAG(res, "long irq (%s) : %fs!!!\n", irq_tag[irqtab[0]],
                    res->clock - irqtime);
        /* end stat stuff */
    }
}
MODULE(kernel, irq_exit);

static void kernel_softirq_entry_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    int id;
    char *s;

    kernel_common(res, pass);
    if (sscanf(res->values, " softirq_id = %d [%m[^]]", &id, &s) != 2) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&sirq[0], (union ltt_value)SOFTIRQ_RUNNING);
        emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_PREEMPT);
        if (id < sizeof(sofirq_tag) && sofirq_tag[id])
            emit_trace(&sirq[1], (union ltt_value)sofirq_tag[id]);
        else
            emit_trace(&sirq[1], (union ltt_value)"softirq %d", id);
        emit_trace(&sirq[2], (union ltt_value)s);
        softirqstate = SOFTIRQS_RUN;
        /* stat stuff */
        strncpy(softirqtask, s, sizeof(softirqtask));
        softirqtask[sizeof(softirqtask)-1] = 0;
        softirqtime = res->clock;
        /* end stat stuff */
    }
    free(s);
}
MODULE(kernel, softirq_entry);

static void kernel_softirq_exit_process(struct ltt_module *mod,
                                        struct parse_result *res, int pass)
{
    kernel_common(res, pass);
    if (pass == 2) {
        if (softirqstate == SOFTIRQS_RAISE)
            emit_trace(&sirq[0], (union ltt_value)SOFTIRQ_RAISING);
        else
            emit_trace(&sirq[0], (union ltt_value)SOFTIRQ_IDLE);
        if (idle_cpu_state == IDLE_RUNNING)
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_RUNNING);
        else
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_IDLE);
        softirqstate = SOFTIRQS_IDLE;
        /* stat stuff */
        /* we allow up to 0.7ms softirq */
        if (res->clock - softirqtime > 0.0007)
            TDIAG(res, "long softirq(%s) :  %fs!!!\n", softirqtask,
                    res->clock - softirqtime);
        /* end stat stuff */
    }
}
MODULE(kernel, softirq_exit);

static void kernel_softirq_raise_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    int id;

    kernel_common(res, pass);
    if (sscanf(res->values, " softirq_id = %d", &id) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        if (softirqstate == SOFTIRQS_IDLE)
            emit_trace(&sirq[0], (union ltt_value)SOFTIRQ_RAISING);
        if (id < sizeof(sofirq_tag) && sofirq_tag[id])
            emit_trace(&sirq[1], (union ltt_value)"! %s", sofirq_tag[id]);
        else
            emit_trace(&sirq[1], (union ltt_value)"raise %d", id);
        softirqstate = SOFTIRQS_RAISE;
    }
}
MODULE(kernel, softirq_raise);

static void kernel_tasklet_low_entry_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    unsigned int data, func;

    kernel_common(res, pass);
    if (sscanf(res->values, " func = 0x%x, data = %u", &func, &data) != 2) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 1) {
        atag_store(func);
    }
    if (pass == 2) {
        emit_trace(&tlow[0], (union ltt_value)LT_S0);
        emit_trace(&tlow[1], (union ltt_value)func);
        if (atag_enabled) {
            strncpy(softirqtask, atag_get(func), sizeof(softirqtask));
            softirqtask[sizeof(softirqtask)-1] = 0;
        }
    }
}
MODULE(kernel, tasklet_low_entry);

static void kernel_tasklet_low_exit_process(struct ltt_module *mod,
                                            struct parse_result *res, int pass)
{
    kernel_common(res, pass);
    if (pass == 1) {
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

    kernel_common(res, pass);
    if (sscanf(res->values, " prev_pid = %u, next_pid = %u, prev_state = %u",
               &prev_pid, &next_pid, &prev_state) != 3) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        struct ltt_trace *current_process;
        current_process = find_task_trace(prev_pid);
        emit_trace(current_process,(union ltt_value)PROCESS_IDLE);
        if (prev_pid == 0) {
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_IDLE);
            idle_cpu_state = IDLE_IDLE;
        }

        current_process = find_task_trace(next_pid);
        /* XXX this is often buggy : some process are in SYCALL mode while running
           in userspace ...
         */
        if (strcmp(res->mode, "USER_MODE") == 0)
            emit_trace(current_process, (union ltt_value)PROCESS_USER);
        else
            emit_trace(current_process, (union ltt_value)PROCESS_KERNEL);
        if (next_pid == 0) {
            emit_cpu_idle_state(res, (union ltt_value)IDLE_CPU_RUNNING);
            idle_cpu_state = IDLE_RUNNING;
        }
    }
}
MODULE(kernel, sched_schedule);

static void kernel_sched_try_wakeup_process(struct ltt_module *mod,
                                          struct parse_result *res, int pass)
{
    unsigned int pid, cpu_id, state;

    kernel_common(res, pass);
    if (sscanf(res->values, " pid = %u, cpu_id = %u, state = %u",
               &pid, &cpu_id, &state) != 3) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&sched_event, (union ltt_value)"%d try wakeup %d", res->pid, pid);
        if (res->pid != pid) {
            struct ltt_trace *next_process;
            next_process = find_task_trace(pid);
            emit_trace(next_process, (union ltt_value)PROCESS_WAKEUP);
        }
    }
}
MODULE(kernel, sched_try_wakeup);

static void kernel_sched_wakeup_new_task_process(struct ltt_module *mod,
                                          struct parse_result *res, int pass)
{
    unsigned int pid, cpu_id, state;

    kernel_common(res, pass);
    if (sscanf(res->values, " pid = %u, state = %u, cpu_id = %u",
               &pid, &state, &cpu_id) != 3) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&sched_event, (union ltt_value)"new task %d", pid);
    }
}
MODULE(kernel, sched_wakeup_new_task);

static void kernel_send_signal_process(struct ltt_module *mod,
                                          struct parse_result *res, int pass)
{
    unsigned int pid, signal;

    kernel_common(res, pass);
    if (sscanf(res->values, " pid = %u, signal = %u",
               &pid, &signal) != 2) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&sched_event, (union ltt_value)"%d send signal %d to pid %d",
                res->pid, signal, pid);
    }
}
MODULE(kernel, send_signal);

static void kernel_syscall_entry_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    unsigned int ip, id;

    kernel_common(res, pass);
    if (sscanf(res->values, " ip = 0x%x, syscall_id = %u",
               &ip, &id) != 2) {
        PARSE_ERROR(mod, res->values);
        return;
    }

    if (pass == 1) {
        atag_store(ip);
    }
    if (pass == 2) {
        struct ltt_trace *current_process;
        current_process = find_task_trace(res->pid);
        emit_trace(current_process, (union ltt_value)PROCESS_KERNEL);
        if (id < sizeof(syscall_name)/sizeof(syscall_name[0]))
            emit_trace(&current_process[1], (union ltt_value)syscall_name[id]);
        else
            emit_trace(&current_process[1], (union ltt_value)"syscall %d", id);
        emit_trace(&syscall_id, (union ltt_value)id);
        emit_trace(&syscall_pc, (union ltt_value)ip);
    }
}
MODULE(kernel, syscall_entry);

static void kernel_syscall_exit_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    int ret;

    kernel_common(res, pass);
    if (sscanf(res->values, " ret = %d",
               &ret) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }

    if (pass == 2) {
        struct ltt_trace *current_process;
        current_process = find_task_trace(res->pid);
        emit_trace(current_process, (union ltt_value)PROCESS_USER);
        /* ret is not valid ... */
        emit_trace(&current_process[1], (union ltt_value)"->%d", ret);
    }
}
MODULE(kernel, syscall_exit);

static void kernel_process_free_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    int pid;

    kernel_common(res, pass);
    if (sscanf(res->values, " pid = %d",
               &pid) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }

    if (pass == 2) {
        struct ltt_trace *current_process;
        current_process = find_task_trace(pid);
        emit_trace(current_process, (union ltt_value)PROCESS_DEAD);
    }
}
MODULE(kernel, process_free);

static void kernel_printk_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    unsigned int ip;

    kernel_common(res, pass);
    if (sscanf(res->values, " ip = 0x%x",
               &ip) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }

    if (pass == 1) {
        atag_store(ip);
    }

    if (pass == 2) {
        emit_trace(&printk_pc, (union ltt_value)ip);
    }
}
MODULE(kernel, printk);

static void kernel_timer_update_time_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    unsigned int c_jiffies;
    unsigned long long c_jiffies64;

    kernel_common(res, pass);
    if (sscanf(res->values, " jiffies = %llu",
               &c_jiffies64) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    c_jiffies = c_jiffies64 & 0xffffffff;

    if (pass == 2) {
		static unsigned int old_jiffies;
		if (old_jiffies && old_jiffies+1 != c_jiffies)
            DIAG("missing jiffies jump from %x to %x (broken trace ?)\n",
				   old_jiffies, c_jiffies);

		old_jiffies = c_jiffies;
        emit_trace(&jiffies, (union ltt_value)c_jiffies);
    }
}
MODULE(kernel, timer_update_time);

static void kernel_thread_setname_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    int id;
    char *s;

    kernel_common(res, pass);
    if (sscanf(res->values, " pid = %d , name = \"%m[^\"]\"", &id, &s) != 2) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 1) {
        find_or_add_task_trace(s, id);
    }
    free(s);
}
MODULE(kernel, thread_setname);

static void kernel_parrot_evt_start_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    int id;

    kernel_common(res, pass);
    if (sscanf(res->values, " id = %d", &id) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&parrot_evt, (union ltt_value)"<-%d", id);
    }
}
MODULE(kernel, parrot_evt_start);

static void kernel_parrot_evt_stop_process(struct ltt_module *mod,
                                         struct parse_result *res, int pass)
{
    int id;

    kernel_common(res, pass);
    if (sscanf(res->values, " id = %d", &id) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 2) {
        emit_trace(&parrot_evt, (union ltt_value)"%d->", id);
    }
}
MODULE(kernel, parrot_evt_stop);
