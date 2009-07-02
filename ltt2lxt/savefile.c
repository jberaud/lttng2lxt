/**
 * @file   savefile.c
 * @brief  LTT to GTKwave trace conversion
 *
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
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

#include "ltt2lxt.h"
#include "savefile.h"

static unsigned int count_traces(void)
{
    unsigned int nb = 0;
    struct ltt_trace *trace;

    for (trace = trace_head(); trace; trace = trace->next) {
        nb++;
    }
    return nb;
}

static int compare_traces(const void *t1, const void *t2)
{
    double d = (*(struct ltt_trace **)t1)->pos-(*(struct ltt_trace **)t2)->pos;
    return (d > 0.0)? 1 : ((d < 0.0)? -1 : 0);
}

static void sort_traces(enum trace_group group, struct ltt_trace **tab,int *len)
{
    int n;
    struct ltt_trace *trace;

    /* filter traces */
    for (trace = trace_head(), n = 0; trace; trace = trace->next) {
        if (trace->group == group && trace->emitted) {
            tab[n++] = trace;
        }
    }
    /* sort traces */
    if (n > 0) {
        qsort(tab, n, sizeof(struct ltt_trace *), &compare_traces);
    }
    *len = n;
}

static void print_group(enum trace_group group, const char *name, FILE *fp,
                        struct ltt_trace **tab)
{
    int i, tablen;
    unsigned int flag;

    sort_traces(group, tab, &tablen);
    if (tablen > 0) {
        fprintf(fp, "@%x\n-%s\n", TR_BLANK, name);

        for (i = 0; i < tablen; i++) {
            flag = 0;
            flag |= (tab[i]->sym->flags & LT_SYM_F_BITS)?    TR_BIN : 0;
            flag |= (tab[i]->sym->flags & LT_SYM_F_INTEGER)? TR_HEX : 0;
            flag |= (tab[i]->sym->flags & LT_SYM_F_STRING)?  TR_ASCII : 0;

            fprintf(fp, "@%x\n%s%s\n", flag|TR_RJUSTIFY, tab[i]->sym->name,
                    (tab[i]->flags & LT_SYM_F_U16)? "[0:15]":"");
        }
    }
}

void write_savefile(const char *name)
{
    unsigned int ntraces;
    struct ltt_trace **tab;
    FILE *fp;

    ntraces = count_traces();
    if (ntraces == 0) {
        return;
    }
    tab = malloc(ntraces*sizeof(struct ltt_trace *));
    assert(tab);

    fp = fopen(name, "wb");
    if (fp == NULL) {
        FATAL("cannot write savefile '%s': %s\n", name, strerror(errno));
    }
    INFO("writing SAV file '%s'...\n", name);

    print_group(TG_IRQ, "Interrupts", fp, tab);
    print_group(TG_MM, "Memory Management", fp, tab);
    print_group(TG_PROCESS, "Processes", fp, tab);

    free(tab);

    fclose(fp);
}
