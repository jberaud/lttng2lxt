/**
 */

#include "ltt2lxt.h"

static struct ltt_trace trace[1];

static void init_traces(void)
{
    init_trace(&trace[0], TG_PROCESS, 0, LT_SYM_F_STRING, "user event");
}

static void userspace_event_process(struct ltt_module *mod,
                            struct parse_result *res, int pass)
{
    char *s;

    if (sscanf(res->values, "  string = \"%m[^\"]\"", &s) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }
    if (pass == 1) {
        init_traces();
    }

    if (pass == 2) {
		emit_trace(&trace[0], (union ltt_value)"%d : \"%s\"",
                   res->pid, s);
    }
	free(s);
}
MODULE(userspace, event);
