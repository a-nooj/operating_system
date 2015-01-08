/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _I8259_H
#define _I8259_H

#include "types.h"

/* Ports that each PIC sits on */
#define MASTER_8259_CMD_PORT 	0x20
#define SLAVE_8259_CMD_PORT  	0xA0
#define MASTER_8259_DATA_PORT 	(MASTER_8259_CMD_PORT + 1) //Added by AJ on 3/6
#define SLAVE_8259_DATA_PORT  	(SLAVE_8259_CMD_PORT  + 1) //Added by AJ on 3/6

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1    	  0x11
#define ICW2_MASTER   0x20
#define ICW2_SLAVE    0x28
#define ICW3_MASTER   0x04
#define ICW3_SLAVE    0x02
#define ICW4          0x01

#define MASK_ALL_IRQ 0xFF
#define UNMASK_MASTER_IRQ2 0xFB

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI             0x60

#define MAX_IRQ 15

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);
/*Sends EOI to all IRQ lines*/
extern void send_eoi_all_irq(void);
/*masks all the irqs, but saves the previous mask */
void mask_all_irq(void);
/*restores the irqs to befor desibale */
void restore_all_irq(void);

#endif /* _I8259_H */
