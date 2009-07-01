/**
 * @file   mod_task_state.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

struct tdata {
    int pid;
    struct ltt_trace *data;
};

static void *root;

static int compare(const void *pa, const void *pb)
{
    int a = ((struct tdata*)pa)->pid;
    int b = ((struct tdata*)pb)->pid;
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

struct ltt_trace * find_task_trace(int pid)
{
    struct tdata tdata, *ret;

    tdata.pid = pid;
    ret = tfind(&tdata, &root, compare);

    assert(ret);
    ret = *((void**)ret);
    return ret->data;
}

struct ltt_trace * find_or_add_task_trace(const char *name, int pid)
{
    struct tdata tdata, *ret;

    tdata.pid = pid;
    ret = tfind(&tdata, &root, compare);

    if (!ret) {
        struct ltt_trace *data;
		/* this can happen if we on the first cs from this process */
        if (!name)
            name = "????";

        //printf("insert pid %d\n", pid);
        data = calloc(2, sizeof(struct ltt_trace));
        assert(data);

        ret = malloc(sizeof(struct tdata));
        assert(ret);
        ret->pid = pid;
        ret->data = data;
        ret = tsearch(ret, &root, compare);
        assert(ret);
        init_trace(&data[0], TG_PROCESS, 1.0 + pid, LT_SYM_F_BITS, "proc.state.%s [%d]", name, pid);
        init_trace(&data[1], /*TG_PROCESS*/0, 1.1 + pid, LT_SYM_F_STRING, "proc.sys.%s [%d] (info)", name, pid);
        ret = *((void**)ret);
    }
    else {
        ret = *((void**)ret);
        refresh_name(&ret->data[0], "proc.state.%s [%d]", name, pid);
        refresh_name(&ret->data[1], "proc.sys.%s [%d] (info)", name, pid);
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
	if (pass == 1) {
		find_or_add_task_trace(s, pid)/*[0].group = 0*/;
	}
    if (pass == 2) {
        //XXX
		if (status == 1) 
        	emit_trace(find_task_trace(pid), (union ltt_value)LT_S0);
    }
}
MODULE(task_state, process_state);
