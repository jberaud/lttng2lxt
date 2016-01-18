/**
 * LTTng to GTKwave trace conversion
 * ardupilot perf module
 *
 * Authors:
 * Julien BERAUD <julien.beraud@parrot.com>
 *
 * Copyright (C) 2013 Parrot S.A.
 */

#include "lttng2lxt.h"
#include "lttng.h"

struct perf_apm {
	char *name;
	uint16_t pos;
	struct ltt_trace trace;
};

static void *root;

static uint16_t perf_count = 0;

static int compare(const void *pa, const void *pb)
{
	char *aname = ((struct perf_apm *)pa)->name;
	char *bname = ((struct perf_apm *)pb)->name;

	return strcmp(aname, bname);
}

static struct perf_apm *new_perf(const char *name)
{
	struct perf_apm *new_perf = calloc(1, sizeof(struct perf_apm));

	assert(new_perf);
	new_perf->name = strdup(name);
	new_perf->pos = perf_count++;

	new_perf = tsearch(new_perf, &root, compare);

	return *((void **)new_perf);
}

static struct perf_apm *find_or_add_perf(const char *name)
{
	struct perf_apm perf, *ret;

	perf.name = strdup(name);
	ret = tfind(&perf, &root, compare);
	free(perf.name);
	if (!ret) {
		ret = new_perf(name);
	} else {
		ret = *((void **)ret);
	}

	return ret;
}

static void ardupilot_begin_process(const char *modname, int pass,
					 double clock, int cpu, void *args)
{
	const char *name = get_arg_str(args, "name_field");
	struct perf_apm *perf = find_or_add_perf(name);

	if (pass == 1) {
		init_trace(&perf->trace,
		   TG_APM,
		   1 + 0.1 * perf->pos,
		   TRACE_SYM_F_BITS,
		   "%s",
		   name);
	}

	if (pass == 2) {
		emit_trace(&perf->trace, (union ltt_value)LT_S0);
	}

}
MODULE2(ardupilot, begin);

static void ardupilot_end_process(const char *modname, int pass,
					 double clock, int cpu, void *args)
{
	const char *name = get_arg_str(args, "name_field");
	struct perf_apm *perf = find_or_add_perf(name);

	if (pass == 1) {
		init_trace(&perf->trace,
		   TG_APM,
		   1 + 0.1 * perf->pos,
		   TRACE_SYM_F_BITS,
		   "%s",
		   name);
	}

	if (pass == 2) {
		emit_trace(&perf->trace, (union ltt_value)LT_IDLE);
	}
}
MODULE2(ardupilot, end);

