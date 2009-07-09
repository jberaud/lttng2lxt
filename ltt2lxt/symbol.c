/**
 * @file   symbol.c
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

static struct ltt_trace *head = NULL;

static int symbol_flushed;

static void insert_symbol(struct ltt_trace *tr)
{
    tr->sym = lt_symbol_find(lt, tr->name);
    if (!tr->sym) {
        uint32_t flags = tr->flags;
        if (tr->flags == LT_SYM_F_ADDR) {
            flags = (atag_enabled)? LT_SYM_F_STRING : LT_SYM_F_INTEGER;
        }
        if (tr->flags == LT_SYM_F_U16) {
            tr->sym = lt_symbol_add(lt, tr->name, 0, 0, 15, 0);
        }
        else {
            tr->sym = lt_symbol_add(lt, tr->name, 0, 0, 0, flags);
        }
        assert(tr->sym);
    }
}

void init_trace(struct ltt_trace *tr,
                enum trace_group group,
                double pos,
                uint32_t flags,
                const char *fmt, ...)
{
    va_list ap;
    static char linebuf[LINEBUF_MAX];

    if (tr->name == NULL) {

        tr->flags = flags;
        tr->group = group;
        tr->pos = pos;
        tr->next = head;
        head = tr;

        va_start(ap, fmt);
        vsnprintf(linebuf, LINEBUF_MAX, fmt, ap);
        va_end(ap);

        tr->name = strdup(linebuf);

        INFO("adding trace '%s' group=%d pos=%g\n", linebuf, group, pos);
        if (symbol_flushed) {
            /* XXX hack late symbol flush */
            insert_symbol(tr);
        }
    }
}

void refresh_name(struct ltt_trace *tr,
                const char *fmt, ...)
{
    va_list ap;
    static char linebuf[LINEBUF_MAX];

    assert(tr->name);

    va_start(ap, fmt);
    vsnprintf(linebuf, LINEBUF_MAX, fmt, ap);
    va_end(ap);

    if (strcmp(tr->name, linebuf)) {
        free((char *)tr->name);
        tr->name = strdup(linebuf);
    }
}

void symbol_flush(void)
{
    struct ltt_trace *tr;

    for (tr = trace_head(); tr; tr = tr->next) {
        insert_symbol(tr);
    }
    symbol_flushed = 1;
}

void emit_trace(struct ltt_trace *tr, union ltt_value value, ...)
{
    va_list ap;
    static char linebuf[LINEBUF_MAX];

    assert(tr->sym);
    tr->emitted = 1;
    switch (tr->flags) {

    case LT_SYM_F_BITS:
        lt_emit_value_bit_string(lt, tr->sym, 0, value.state);
        break;

    case LT_SYM_F_U16:
        assert(value.data <= 0xffff);
    case LT_SYM_F_INTEGER:
        lt_emit_value_int(lt, tr->sym, 0, value.data);
        break;

    case LT_SYM_F_STRING:
        va_start(ap, value);
        vsnprintf(linebuf, LINEBUF_MAX, value.format, ap);
        va_end(ap);
        lt_emit_value_string(lt, tr->sym, 0, linebuf);
        break;

    case LT_SYM_F_ADDR:
        if (tr->sym->flags == LT_SYM_F_STRING) {
            lt_emit_value_string(lt, tr->sym, 0, atag_get(value.data));
        }
        else {
            lt_emit_value_int(lt, tr->sym, 0, value.data);
        }
        break;
    default:
        assert(0);
    }
}

struct ltt_trace *trace_head(void)
{
    return head;
}

void emit_clock(double clock)
{
    lxttime_t timeval;
    static lxttime_t oldtimeval = 0;

    // XXX hardcoded timescale
    timeval = (lxttime_t)(1000000000.0*clock);
    if (timeval <= oldtimeval) {
        DIAG("negative time offset @%lld: %d !\n", oldtimeval,
             (int)((int64_t)timeval-(int64_t)oldtimeval));
        timeval = oldtimeval + 1;
    }

    oldtimeval = timeval;
    lt_set_time64(lt, timeval);
}
