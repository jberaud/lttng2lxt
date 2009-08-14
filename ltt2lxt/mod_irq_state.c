/**
 * @file   mod_irq_state.c
 * @author ivan.djelic@parrot.com
 * @date   2009-06-18
 */

#include "ltt2lxt.h"

//XXX default to P6 irq vectors
char *irq_tag[MAX_IRQS] = {
	"LCDC",
	"CAN",
	"AAI",
	"MOST",
	"SDIO",
	"UART0",
	"UART1",
	"UART2",
	"UART3",
	"UARTSIM0",
	"NANDMC",
	"SPIX",
	"I2CS",
	"GPIO",
	"DMAC",
	"TICK",
	"TIMER0",
	"TIMER1",
	"TIMER2",
	"TIMER3",
	"H264",
	"CAMIF0",
	"CAMIF1",
	"CV",
	"MON",
	"PARINT",
	"COMMRX",
	"COMMTX",
	"I2CM",
	"USB0",
	"USB1",
	"MMCIX",
	[32 ... MAX_IRQS-1] = "gpio",
};

static void irq_state_interrupt_process(struct ltt_module *mod,
                                        struct parse_result *res, int pass)
{
    char *s1, *s2;
    unsigned int irq;

	if (pass == 1) {
        if (sscanf(res->values,
                   " name = %m[^,], action = \"%m[^\"]\", irq_id = %u",
                   &s1, &s2, &irq) != 3) {
            PARSE_ERROR(mod, res->values);
		return;
        }
        free(s1);
        if (irq < MAX_IRQS) {
            irq_tag[irq] = s2;
        }
        else {
            free(s2);
        }
	}
}
MODULE(irq_state, interrupt);
