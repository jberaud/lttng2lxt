/**
 * @file   parse.c
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

#define _CH           "\\([^\\.]\\+\\)"
#define _N            "\\([^\\:]\\+\\)"
#define _CLK          "\\([0-9\\.]\\+\\)"
#define _FILE         " ([^)]*),"
#define _PID          " \\([0-9]\\+\\),"
#define _PNAME        " \\([^,]*\\),"
#define _HEX          " \\(0x[0-9A-Fa-f]\\),"
#define _MODE         " \\([A-Z_]\\+\\)"
#define _VAL          " {\\([^}]\\+\\) }"

#define LINE_REGEX                                                      \
    "^" _CH "." _N ": " _CLK _FILE _PID _PID _PNAME " ," _PID _HEX _MODE _VAL

#define LINE_MATCHES  (11)

static regex_t line_preg;

void parse_init(void)
{
	int ret = regcomp(&line_preg, LINE_REGEX, 0);
	assert(ret == 0);
}

int parse_line(const char *line, struct parse_result *res)
{
	int i, ret;
	const char *channel;
	const char *name;
	regmatch_t match[LINE_MATCHES];
	static char *smatch[LINE_MATCHES] = {NULL, NULL, NULL, NULL, NULL};

	/* TODO this is very slow (take 75% of time), and should replaced by a more simple
	   token parser
	 */
	ret = regexec(&line_preg, line, LINE_MATCHES, match, 0);

	if ((ret == REG_NOMATCH) || (match[LINE_MATCHES-1].rm_so == -1)) {
		/* no match */
		return -1;
	}
	/* store match strings */
	for (i = 1; i < LINE_MATCHES; i++) {
		if (smatch[i]) {
			free(smatch[i]);
		}
		smatch[i] = strndup(line + match[i].rm_so,
							match[i].rm_eo-match[i].rm_so);
		assert(smatch[i]);
	}
	channel = smatch[1];
	name = smatch[2];
	res->clock = atof(smatch[3]);
	res->pid = atoi(smatch[4]);
	res->pname = clean_name(smatch[6]);
	res->mode = smatch[9];
	res->values = smatch[10];

	res->module = find_module_by_name(channel, name);

	if (res->pname[0] == 0) {
		res->pname = "no name";
	}

	return (res->module)? 0 : -1;
}
