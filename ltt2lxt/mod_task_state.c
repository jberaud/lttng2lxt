/**
 * @file   mod_task_state.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

struct ltt_trace * find_or_add_task_trace(const char *name, int pid)
{
    int status;
    ENTRY entry, *ret;
    static int init = 0;
    static struct hsearch_data table;

    if (!init) {
        memset(&table, 0, sizeof(struct hsearch_data));
        status = hcreate_r(100, &table);
        assert(status);
        init = 1;
    }

    entry.key = (char *)strdup(name);
    assert(entry.key);
    (void)hsearch_r(entry, FIND, &ret, &table);

    if (!ret) {
        entry.data = calloc(1, sizeof(struct ltt_trace));
        assert(entry.data);
        status = hsearch_r(entry, ENTER, &ret, &table);
        assert(status);
        init_trace(entry.data, TG_PROCESS, 1.0 + pid, LT_SYM_F_BITS, name);
        ret = &entry;
    }
    else {
        free(entry.key);
    }

    return ret->data;
}

static void task_state_process_state_process(struct ltt_module *mod,
                                             struct parse_result *res, int pass)
{
    char *s;
	uint32_t pid, parent_pid, type, mode, submode, status, tgid;

	if (sscanf(res->values, " pid = %d, parent_pid = %d, name = \"%m[^\"]\", "
               "type = %d, mode = %d, submode = %d, status = %d, tgid = %d",
			   &pid, &parent_pid, &s, &type, &mode, &submode, &status,
               &tgid) != 8) {
		PARSE_ERROR(mod, res->values);
		return;
	}
	if (pass == 2) {
        //XXX
        //char *level = (status == 1)? LT_S0 : LT_IDLE;
		//emit_trace(find_or_add_task_trace(s, pid), (union ltt_value)level);
	}
}
MODULE(task_state, process_state);
