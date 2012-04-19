/**
 */

#include "ltt2lxt.h"

static struct ltt_trace trace_g;
static struct ltt_trace trace[32];

static void init_traces(void)
{
    init_trace(&trace_g, TG_PROCESS, 0.1, TRACE_SYM_F_STRING, "user event");
}

static void userspace_event_process(struct ltt_module *mod,
                            struct parse_result *res, int pass)
{
    char *s;
    int num = -1;

    if (sscanf(res->values, "  string = \"%m[^\"]\"", &s) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }

    if (strlen(s) > 2) {
        sscanf(s+2, "%d", &num);
    }
    if (pass == 1) {
        init_traces();
        if (num < sizeof(trace)/sizeof(trace[0]) && num >= 0)
            init_trace(&trace[num], TG_PROCESS, 0.1+0.1*num, TRACE_SYM_F_BITS, "user event %d", num);
    }

    if (pass == 2) {
        emit_trace(&trace_g, (union ltt_value)"%d : \"%s\"",
                   res->pid, s);
        if (num < sizeof(trace)/sizeof(trace[0]) && num >= 0) {
            if (s[0] == 'S')
                emit_trace(&trace[num], (union ltt_value)LT_S0);
            else
                emit_trace(&trace[num], (union ltt_value)LT_IDLE);
        }
    }
    free(s);
}
MODULE(userspace, event);
