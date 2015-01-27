/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
	uint32_t flags;

	cli_and_save(flags);

	master_mask = MASK_ALL;
	slave_mask = MASK_ALL;

	//mask all interrupts
	outb(master_mask, MASTER_8259_DATA);
	outb(slave_mask, SLAVE_8259_DATA);

	//send control words to master
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_8259_DATA);
	outb(ICW3_MASTER, MASTER_8259_DATA);
	outb(ICW4, MASTER_8259_DATA);

	//send control words to slave
	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_DATA);
	outb(ICW3_SLAVE, SLAVE_8259_DATA);
	outb(ICW4, SLAVE_8259_DATA);

	outb(master_mask, MASTER_8259_DATA); //restore master IRQ mask
	outb(slave_mask, SLAVE_8259_DATA); //restore slave IRQ mask

	restore_flags(flags);
}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	uint8_t generic_mask;

	if (irq_num < NUM_IRQS) {
		generic_mask = ~(1 << irq_num);
		master_mask = (uint8_t) inb(MASTER_8259_DATA) & generic_mask;
		outb(master_mask, MASTER_8259_DATA);
	}

	else {
		irq_num = irq_num - NUM_IRQS;
		generic_mask = ~(1 << irq_num);
		slave_mask = (uint8_t) inb(SLAVE_8259_DATA) & generic_mask;
		outb(slave_mask, SLAVE_8259_DATA);
	}
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
	uint8_t generic_mask;
	
	if (irq_num < NUM_IRQS) {
		generic_mask = (1 << irq_num);
		master_mask = (uint8_t) inb(MASTER_8259_DATA) | generic_mask;
		outb(master_mask, MASTER_8259_DATA);
	}

	else {
		irq_num = irq_num - NUM_IRQS;
		generic_mask = (1 << irq_num);
		slave_mask = (uint8_t) inb(SLAVE_8259_DATA) | generic_mask;
		outb(slave_mask, SLAVE_8259_DATA);
	}
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
	if (irq_num & 8) {
		outb(EOI + (irq_num & 7), SLAVE_8259_PORT);
		outb(EOI + ICW3_SLAVE, MASTER_8259_PORT);
	}
	
	else {
		outb(EOI + irq_num, MASTER_8259_PORT);
	}
}
