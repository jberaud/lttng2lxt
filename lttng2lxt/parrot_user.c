/**
 * LTTng to GTKwave trace conversion
 *
 * Authors:
 * Ivan Djelic <ivan.djelic@parrot.com>
 * Matthieu Castet <matthieu.castet@parrot.com>
 *
 * Copyright (C) 2013 Parrot S.A.
 */

#include "lttng2lxt.h"
#include "lttng.h"

#define MAX_USER_EVENTS     32

static struct ltt_trace trace_g;
static struct ltt_trace traces[MAX_USER_EVENTS];

/*static void init_traces(void)
{
    init_trace(&trace_g, TG_PROCESS, 0.1, TRACE_SYM_F_STRING, "user event");
}*/

static void parrot_event_start_process(int pass, double clock, int cpu,
                                       union arg_value *args[MAX_ARGS])
{
    int num = (int)args[0]->i64;

    if (pass == 1) {
        if (num < (int)(sizeof(traces)/sizeof(traces[0])) && num >= 0)
            init_trace(&traces[num], TG_PROCESS, 0.1+0.1*num, TRACE_SYM_F_BITS, "user event %d", num);
    }

    if (pass == 2) {
        if (num < (int)(sizeof(traces)/sizeof(traces[0])) && num >= 0) {
            emit_trace(&traces[num], (union ltt_value)LT_S0);
        }
    }
}
MODULE2(parrot, event_start, "event_start");

static void parrot_event_stop_process(int pass, double clock, int cpu,
                                      union arg_value *args[MAX_ARGS])
{
    int num = (int)args[0]->i64;

    if (pass == 1) {
        if (num < (int)(sizeof(traces)/sizeof(traces[0])) && num >= 0)
            init_trace(&traces[num], TG_PROCESS, 0.1+0.1*num, TRACE_SYM_F_BITS, "user event %d", num);
    }

    if (pass == 2) {
        if (num < (int)(sizeof(traces)/sizeof(traces[0])) && num >= 0) {
            emit_trace(&traces[num], (union ltt_value)LT_IDLE);
        }
    }
}
MODULE2(parrot, event_stop, "event_stop");

static void parrot_message_process(int pass, double clock, int cpu,
                                   union arg_value *args[MAX_ARGS])
{
    const char * str = args[0]->s;

    if (pass == 1)
        init_trace(&trace_g, TG_PROCESS, 0.1, TRACE_SYM_F_STRING, "user event");

    if (pass == 2)
         emit_trace(&trace_g, (union ltt_value)"%s", str);

}
MODULE2(parrot, message, "message");

