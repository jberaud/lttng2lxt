/**
 * @file   mod_fs.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

static void fs_exec_process(struct ltt_module *mod,
                            struct parse_result *res, int pass)
{
    char *s;

    if (sscanf(res->values, " filename = \"%m[^\"]\"", &s) != 1) {
        PARSE_ERROR(mod, res->values);
        return;
    }

	/*
    if (pass == 1) {
        find_or_add_task_trace(s, res->pid, pass);
    }

    if ((pass == 2) && (current_process)) {
        if (current_process) {
            emit_trace(current_process,(union ltt_value)LT_IDLE);
        }
        current_process = find_or_add_task_trace(s, res->pid, pass);
        emit_trace(current_process, (union ltt_value)LT_S0);
    }
	*/
}
MODULE(fs, exec);
