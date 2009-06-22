/**
 * @file   ltt2lxt.h
 * @brief  LTT to GTKwave trace conversion
 *
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 *
 * This utility is used to produce LXT dumpfiles (viewable with GTKwave) from
 * text dumps of LTT traces.
 *
 * Usage: ltt2lxt [-v] [-e <exefile>] <lttdump> [<lxtfile> <savefile>]
 * Copyright (C) 2009 Parrot S.A.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef LTT2LXT_H
#define LTT2LXT_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <regex.h>
#include <getopt.h>
#include <search.h>

#include "lxt_write.h"

#define PFX "ltt2lxt: "

#define LINEBUF_MAX             (512)

#define LT_S0                   "x"
#define LT_S1                   "u"
#define LT_S2                   "w"
#define LT_IDLE                 "z"
#define LT_1                    "1"
#define LT_0                    "0"

#define MAX_IRQS                (32)

enum trace_group {
    TG_IRQ,
    TG_MM,
    TG_PROCESS,
};

//! Additional pseudo-trace symbol type
#define LT_SYM_F_ADDR          (1<<31)

struct ltt_trace {
    struct lt_symbol *sym;
    uint32_t          flags;
    enum trace_group  group;
    double            pos;
    struct ltt_trace *next;
};

struct parse_result {
	struct ltt_module *module;
	double             clock;
    int                pid;
    const char        *pname;
    const char        *mode;
	const char        *values;
};

struct ltt_module {
	const char     *channel;
	const char     *name;
	void          (*process)(struct ltt_module *, struct parse_result *, int);
};

union ltt_value {
    char       *state;
    uint32_t    data;
    const char *format;
};

#define __str(_x)       #_x
#define __xstr(_x)      __str(_x)
#define MODSECT(_n_)    __attribute__((section(__xstr(.data.ltt2lxt.##_n_))))

#define MODULE(_chan_,_name_)                         \
	struct ltt_module __ ## _chan_ ## _ ## _name_     \
    MODSECT(1 ## _chan_ ## _ ## _name_) = {           \
		.channel = #_chan_,                           \
		.name    = #_name_,                           \
		.process = _chan_ ## _ ## _name_ ## _process, \
	}

#define FATAL(_fmt, args...)                    \
	do {                                        \
		fprintf(stderr, PFX _fmt, ##args);      \
		exit(1);                                \
	} while (0)

#define INFO(_fmt, args...)                     \
	do {                                        \
		if (verbose) {                          \
			fprintf(stderr, PFX _fmt, ##args);	\
		}                                       \
	} while (0)

#define DIAG(_fmt, args...)                     \
	do {                                        \
		fprintf(stderr, PFX _fmt, ##args);      \
	} while (0)

#define PARSE_ERROR(_mod_,_val_)										\
	DIAG("%s.%s: unable to parse values '%s'\n",						\
		 (_mod_)->channel, (_mod_)->name, _val_)

extern struct lt_trace *lt;
extern int verbose;
extern int atag_enabled;
extern char *irq_tag[MAX_IRQS];
extern struct ltt_trace *current_process;

void atag_init(const char *name);
char *atag_get(uint32_t addr);
void atag_store(uint32_t addr);
void atag_flush(void);

void init_trace(struct ltt_trace *tr,
                enum trace_group group,
                double pos,
                uint32_t flags,
                const char *fmt, ...);

void emit_trace(struct ltt_trace *tr, union ltt_value value, ...);
struct ltt_trace *trace_head(void);
void emit_clock(double clock);
struct ltt_trace * find_or_add_task_trace(const char *name, int pid);

void parse_init(void);
int parse_line(const char *line, struct parse_result *res);

void modules_init(void);
struct ltt_module *find_module_by_name(const char *channel, const char *name);

void write_savefile(const char *name);

#endif /* LTT2LXT_H */
