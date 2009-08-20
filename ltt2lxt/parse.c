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

void parse_init(void)
{
}

#define PARSE(_line, _tok, _name) \
	do { \
	_line = strchr(_line, _tok); \
	if (_line == NULL) \
		return -1; \
	*_line = '\0'; \
	_line++; \
	if (*_line == ' ') \
		_line++; \
	_name = _line; \
	} while(0)

int parse_line(char *line, struct parse_result *res)
{
	const char *channel;
	const char *name;
	const char *clock;
	const char *pid;
	char *pname;
	const char *mode;
	const char *values;
	const char *dummy;


	channel = line;
	PARSE(line, '.', name);
	PARSE(line, ':', clock);
	PARSE(line, ',', pid);
	PARSE(line, ',', dummy);
	PARSE(line, ',', pname);
	PARSE(line, ',', dummy);
	PARSE(line, ',', dummy);
	PARSE(line, ',', mode);
	PARSE(line, ' ', dummy);
	PARSE(line, '{', values);
	PARSE(line, '}', dummy);
	assert(strcmp(line, "\n") == 0);

	res->clock = atof(clock);
	res->pid = atoi(pid);
	res->pname = clean_name(pname);
	res->mode = mode;
	res->values = values;

	res->module = find_module_by_name(channel, name);

	if (res->pname[0] == 0) {
		res->pname = "no name";
	}

	return (res->module)? 0 : -1;
}
