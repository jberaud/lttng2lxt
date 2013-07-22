/**
 * LTTng to GTKwave trace conversion
 *
 * Authors:
 * Ivan Djelic <ivan.djelic@parrot.com>
 * Matthieu Castet <matthieu.castet@parrot.com>
 *
 * Copyright (C) 2013 Parrot S.A.
 */
#ifndef LTTNG2LXT_H
#define LTTNG2LXT_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <regex.h>
#include <getopt.h>
#include <search.h>

enum {
	TRACE_SYM_F_BITS,
	TRACE_SYM_F_INTEGER,
	TRACE_SYM_F_STRING,
	TRACE_SYM_F_DOUBLE,
};

//! Additional pseudo-trace symbol type
#define TRACE_SYM_F_ANALOG        ((1U<<29)|TRACE_SYM_F_DOUBLE)
#define TRACE_SYM_F_U16           ((1U<<30)|TRACE_SYM_F_INTEGER)
#define TRACE_SYM_F_ADDR          (1U<<31)

#define PFX "lttng2lxt: "

#define LINEBUF_MAX               (1024)

#define LT_S0                     "x"
#define LT_S1                     "u"
#define LT_S2                     "w"
#define LT_IDLE                   "z"
#define LT_1                      "1"
#define LT_0                      "0"

#define MAX_CPU                   (4)
#define MAX_IRQS                  (1024)

enum trace_group {
	TG_NONE,
	TG_IRQ,
	TG_MM,
	TG_PROCESS,
};

union ltt_value {
	char       *state;
	uint32_t    data;
	const char *format;
	double      dataf;
};

struct ltt_trace {
	void             *sym;
	uint32_t          flags;
	enum trace_group  group;
	double            pos;
	const char       *name;
	int               emitted;
	struct ltt_trace *next;
	/* XXX alow to save the task state before cs. should be done
	   in another struct */
	union ltt_value value;
};

struct parse_result {
	struct ltt_module *module;
	double             clock;
	int                pid;
	const char        *pname;
	const char        *mode;
	const char        *values;
};

#define MAX_ARGS             (8)

union arg_value {
	uint64_t    u64;
	int64_t     i64;
	const char *s;
};

struct ltt_module {
	const char  *name;
	const char  *args[MAX_ARGS];
	void       (*process)(int pass, double clock, int cpu_id,
			      union arg_value *args[MAX_ARGS]);
} __attribute__((aligned(64)));

#define __str(_x)       #_x
#define __xstr(_x)      __str(_x)
#define MODSECT(_n_)    __attribute__((section(__xstr(.data.lttng2lxt.##_n_))))

#define MODULE(_name_, ...)						\
	MODSECT(1_ ## _name_) struct ltt_module __ ## _name_ = {	\
		.name    = #_name_,					\
		.args    = {__VA_ARGS__},				\
		.process = _name_ ## _process,				\
	}

#define FATAL(_fmt, args...)				\
	do {						\
		fprintf(stderr, PFX _fmt, ##args);      \
		exit(1);                                \
	} while (0)

#define INFO(_fmt, args...)					\
	do {							\
		if (verbose) {					\
			fprintf(stderr, PFX _fmt, ##args);	\
		}						\
	} while (0)

#define DIAG(_fmt, args...) fprintf(stderr, PFX _fmt, ##args)

#define TDIAG(_name, _clock,  _fmt, args...)				\
	do {								\
		if (diag) {						\
			fprintf(stderr, PFX "%s\t@%.0f ns :",		\
				(_name), (_clock)*1000000000);		\
			fprintf(stderr, _fmt, ##args);			\
		}							\
	} while (0)

#define PARSE_ERROR(_m,_v) DIAG("%s: cannot parse values '%s'\n",(_m)->name, _v)

extern int verbose;
extern int diag;
extern int gtkwave_parrot;
extern int atag_enabled;

void atag_init(const char *name);
char *atag_get(uint32_t addr);
void atag_store(uint32_t addr);
void atag_flush(void);

void init_trace(struct ltt_trace *tr,
                enum trace_group group,
                double pos,
                uint32_t flags,
                const char *fmt, ...);
void refresh_name(struct ltt_trace *tr,
		  const char *fmt, ...);
void symbol_flush(void);

void emit_trace(struct ltt_trace *tr, union ltt_value value, ...);
struct ltt_trace *trace_head(void);
void emit_clock(double clock);
void save_dump_init(const char *name);
void save_dump_close(void);

struct ltt_trace * find_or_add_task_trace(const char *name, int pid, int tgid);
struct ltt_trace * find_task_trace(int pid);

void parse_init(void);
int parse_line(char *line, struct parse_result *res);

void modules_init(void);
struct ltt_module *find_module_by_name(const char *name);

void write_savefile(const char *name);
void scan_lttng_trace(const char *name);

void set_cpu_idle(double clock, int cpu);
void set_cpu_running(double clock, int cpu);
void cpu_preempt(double clock, int cpu);
void cpu_unpreempt(double clock, int cpu);

void symbol_clean_name(char *name);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) ((int)(sizeof(x)/sizeof((x)[0])))
#endif

#endif /* LTTNG2LXT_H */
