/**
 * @file   atag.c
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

#define HASHTAB_SIZE            (100000)
#define ADDR2LINE_MAX           (128)

int atag_enabled = 0;
static const char *exefile = NULL;

void atag_init(const char *name)
{
	int res;

	exefile = name;

    if (exefile) {
        /* create a new hash table */
        res = hcreate(HASHTAB_SIZE);
        assert(res);
		atag_enabled = 1;
    }
}

static void atag_enqueue_addr(uint32_t addr, int flush)
{
    static char cmdbuf[2*LINEBUF_MAX + ADDR2LINE_MAX*11];
    static char funcname[LINEBUF_MAX];
    static char basename[LINEBUF_MAX];
    static char tag[LINEBUF_MAX];

    static int index = 0;
    static uint32_t addrbuf[ADDR2LINE_MAX];

    int i;
    char hex32buf[12];
    char key[16];
    FILE *fp;

    ENTRY item, *rentry;

	if (!exefile) {
		return;
	}

    if (!flush) {
        // check that this address is not already queued
        for (i = 0; i < index; i++) {
            if (addrbuf[i] == addr) {
                // already in
                return;
            }
        }
        addrbuf[index++] = addr;
    }

    if ((index >= ADDR2LINE_MAX)||(flush && index)) {

        // prepare command string
        snprintf(cmdbuf, LINEBUF_MAX, "addr2line -C -f -s --exe=%s ", exefile);
        // pass arguments
        assert(index <= ADDR2LINE_MAX);

        for (i = 0; i < index; i++) {
            snprintf(hex32buf, 12, "0x%08x ", (unsigned int)addrbuf[i]);
            strncat(cmdbuf, hex32buf, 11);
        }

        // call addr2line now
        fp = popen(cmdbuf, "r");
        if (fp) {

            for (i = 0; i < index; i++) {
                char *ret = NULL;

                if (fgets(funcname, LINEBUF_MAX, fp) &&
					fgets(basename, LINEBUF_MAX, fp)) {
                    char *str;
                    // remove newline characters
                    str = strchr(funcname, '\n');
                    if (str) {
                        *str = '\0';
                    }
                    str = strchr(basename, '\n');
                    if (str) {
                        *str = '\0';
                    }
					if (funcname[0] != '?') {
						snprintf(tag, LINEBUF_MAX, "%s() [%s]", funcname,
								 basename);
					}
					else {
						snprintf(tag, LINEBUF_MAX, "0x%08x", addrbuf[i]);
					}
                    ret = strdup(tag);
                }

                if (!ret) {
                    fprintf(stderr, PFX "no symbol for addr 0x%08x: %s\n",
							(unsigned int)addrbuf[i], strerror(errno));
                }
                else {
                    //  add entry into hash table
                    snprintf(key, sizeof(key), "%08x", addrbuf[i]);
                    item.key = strdup(key);
                    item.data = (void *)ret;
                    rentry = hsearch(item, ENTER);
                    assert(rentry);
                }
            }
            pclose(fp);
        }
        index = 0;
    }
}

//! Get a symbol name from an address
char * atag_get(uint32_t addr)
{
    char *ret = NULL;
    char key[16];
    ENTRY item, *fitem;

    if (exefile) {
	snprintf(key, sizeof(key), "%08x", addr);
        // lookup address info in hash table
        item.key = key;
        fitem = hsearch(item, FIND);
        if (fitem) {
            ret = (char *)fitem->data;
        }
    }
    return ret;
}

void atag_store(uint32_t addr)
{
    if (!atag_get(addr)) {
        atag_enqueue_addr(addr, 0);
    }
}

void atag_flush(void)
{
    atag_enqueue_addr(0, 1);
}
