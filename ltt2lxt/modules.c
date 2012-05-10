/**
 * @file   modules.c
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

static struct ltt_module __modules_begin MODSECT(0);
static struct ltt_module __modules_end   MODSECT(2);
static struct ltt_module *modtab = &__modules_begin+1;
static struct hsearch_data table;

static char *mk_module_key(const char *channel, const char *name)
{
	char *ret;
	unsigned int len;

	len = strlen(channel) + strlen(name) + 1;
	ret = malloc(len);
	assert(ret);
	strcpy(ret, channel);
	strcat(ret, name);

	return ret;
}

void modules_init(void)
{
	unsigned int modcnt;
	int i, status;
	ENTRY entry, *ret;

	status = hcreate_r(100, &table);
	assert(status);

    modcnt = &__modules_end-&__modules_begin-1;
	INFO("modules (%d):", modcnt);

	for (i = 0; i < modcnt; i++) {
		entry.key = mk_module_key(modtab[i].channel, modtab[i].name);
		entry.data = &modtab[i];
		status = hsearch_r(entry, ENTER, &ret, &table);
        assert(status);
		if (verbose) {
			fprintf(stderr, " %s.%s", modtab[i].channel, modtab[i].name);
		}
	}
	if (verbose) {
		fprintf(stderr,"\n");
	}
}

struct ltt_module *find_module_by_name(const char *channel, const char *name)
{
	ENTRY entry, *ret;

	entry.key = mk_module_key(channel, name);
	(void)hsearch_r(entry, FIND, &ret, &table);
	free(entry.key);

	if (ret) {
		return ret->data;
	}
	if (strcmp(channel, "kernel") == 0) {
		INFO("no support for %s.%s\n", channel, name);
	}
    return NULL;
}
