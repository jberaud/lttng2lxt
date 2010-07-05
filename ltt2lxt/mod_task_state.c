/**
 * @file   mod_task_state.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"
#define PROCESS_STATE "proc.state.[%d-%d] %s"
#define PROCESS_INFO "proc.sys.[%d-%d] %s (info)"

struct tdata {
    int pid;
    int tgid;
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

	/* XXX big hack
	   This should be removed we use alias to track name */
    if (ret == NULL) {
		struct ltt_trace *r;
        r =  find_or_add_task_trace(NULL, pid, 0);
		return r;
    }
	assert(ret);
    ret = *((void**)ret);
    return ret->data;
}

struct ltt_trace * find_or_add_task_trace(const char *name, int pid, int tgid)
{
    struct tdata tdata, *ret;

    tdata.pid = pid;
    ret = tfind(&tdata, &root, compare);

    if (name && strcmp(name, "swapper") == 0) {
        name = "idle thread";
        tgid = 0;
    }
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
        ret->tgid = tgid;
        ret->data = data;
        ret = tsearch(ret, &root, compare);
        assert(ret);
        init_trace(&data[0], TG_PROCESS, 1.0 + (tgid<<16) + pid, LT_SYM_F_BITS, PROCESS_STATE, tgid, pid, name);
        init_trace(&data[1], /*TG_PROCESS*/0, 1.1 + (tgid<<16) + pid, LT_SYM_F_STRING, PROCESS_INFO, tgid, pid, name);
        ret = *((void**)ret);
    }
    else if (strcmp(name, "no name") != 0 &&
			strcmp(name, "kthreadd") != 0 /* XXX */
			) {
        ret = *((void**)ret);
        if (tgid == 0 && ret->tgid != 0) {
            tgid = ret->tgid;
        }
        refresh_name(&ret->data[0], PROCESS_STATE, tgid, pid, name);
        refresh_name(&ret->data[1], PROCESS_INFO, tgid, pid, name);
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
		find_or_add_task_trace(clean_name(s), pid, tgid)/*[0].group = 0*/;
	}
    if (pass == 2) {
        //XXX
		if (status == 1) 
        	emit_trace(find_task_trace(pid), (union ltt_value)LT_S0);
    }
	free(s);
}
MODULE(task_state, process_state);
