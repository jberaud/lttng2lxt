/**
 * @file   ltt2lxt.c
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

#include "ltt2lxt.h"
#include <unistd.h>

int verbose = 0;
int diag = 0;
int gtkwave_parrot = 0;

static void scan_lttdump(const char *name)
{
    int status;
    FILE *fp;
    struct parse_result res;
    static char buf[LINEBUF_MAX];

    fp = fopen(name, "rb");
    if (!fp) {
        FATAL("cannot open '%s': %s\n", name, strerror(errno));
    }

    INFO("pass 1: initializing modules and converting address to symbols\n");

    while (fgets(buf, LINEBUF_MAX-1, fp) != NULL) {
        status = parse_line(buf, &res);
        if (status == 0) {
			res.module->process(res.module, &res, 1);
        }
        else if (strcmp(buf, "End trace set\n") == 0) {
            break;
        }
    }
	/* flush address symbol conversion pipe */
	atag_flush();

    symbol_flush();
    INFO("pass 2: emitting LXT traces\n");

	rewind(fp);

    while (fgets(buf, LINEBUF_MAX-1, fp) != NULL) {
        status = parse_line(buf, &res);
        if (status == 0) {
			/* emit LXT traces */
			emit_clock(res.clock);
			INFO("%s.%s @ %g (%s)\n", res.module->channel, res.module->name,
				 res.clock, res.values);
			res.module->process(res.module, &res, 2);
        }
        else if (strcmp(buf, "End trace set\n") == 0) {
            break;
        }
    }

    fclose(fp);
}

static void usage(void)
{
    fprintf (stderr, "\nUsage: ltt2lxt [-v] [-d] [-c] [-e <exefile>] "
			 "<lttdump> [<lxtfile> <savefile>]\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int c;
    const char *dumpfile;
    char *lxtfile;
    char *savefile;
    //uint32_t step, next_update;

    while ((c = getopt(argc, argv, "hvdce:")) != -1) {
        switch (c) {

        case 'e':
            atag_init(optarg);
            break;

        case 'v':
            verbose = 1;
            break;

        case 'd':
            diag = 1;
            break;

        case 'c':
            gtkwave_parrot = 1;
            break;

        case 'h':
        default:
            usage();
            break;
        }
    }

    if ((optind != argc-1) && (argc != argc-3)) {
        usage();
    }

    dumpfile = argv[optind];

    if (optind == argc-3) {
        lxtfile = argv[optind+1];
        savefile = argv[optind+2];
    }
    else {
        /* make new names with proper extensions */
        lxtfile = (char *)malloc(strlen(dumpfile)+5);
        savefile = (char *)malloc(strlen(dumpfile)+6);
        assert(lxtfile && savefile);
        strcpy(lxtfile, dumpfile);
        strcat(lxtfile, ".lxt");
        strcpy(savefile, dumpfile);
        strcat(savefile, ".sav");
    }

    modules_init();
    parse_init();
    save_dump_init(lxtfile);

    scan_lttdump(dumpfile);

    // create a savefile for GTKwave with comments, trace ordering, etc.
    write_savefile(savefile);
    strcpy(lxtfile, savefile);
    strcpy(savefile, dumpfile);
    strcat(savefile, ".gtkw");
    unlink(savefile);
    link(lxtfile, savefile);

    save_dump_close();

    if (optind != argc-3) {
        free(lxtfile);
        free(savefile);
    }

    return 0;
}
