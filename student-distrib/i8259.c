/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"


/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* i8259_init
 * DESCRIPTION: sends all bytes to initalize the i8259 chip
 * INPUTS: none
 * OUTPUTS: lots of bytes to different ports
 * RETURN VAL: none
 */
void
i8259_init(void)
{
	//NOTE: we might want to add wait() instructions...

	//mask out all interrupts on both PICs before we do anything else
	outb(MASK_ALL_IRQ, MASTER_8259_DATA_PORT);
	outb(MASK_ALL_IRQ, SLAVE_8259_DATA_PORT);
	
	//Send the init control words to the PICs
	outb(ICW1, MASTER_8259_CMD_PORT);
	outb(ICW1, SLAVE_8259_CMD_PORT);

	//Set PIC modes (master/slave)
	outb(ICW2_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_DATA_PORT);
	
	outb(ICW3_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW3_SLAVE, SLAVE_8259_DATA_PORT);	
	
	//x86 mode
	outb(ICW4, MASTER_8259_DATA_PORT);
	outb(ICW4, SLAVE_8259_DATA_PORT);

	//unmask IRQ2 on master to allow for cascading from slave PIC
	outb(UNMASK_MASTER_IRQ2, MASTER_8259_DATA_PORT);

}

/* enable_irq
 * DESCRIPTION: enables a specified IRQ line by sending bytes to the approprate PIC(s)
 * INPUTS: irq_num - the IRQ line to alter
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void
enable_irq(uint32_t irq_num)
{
	uint8_t mask = 0x01 << irq_num;
	mask = ~mask; //We're disabling the mask

	//NOTE: might need a spinlock here...
	
	//Check if the IRQ is on the master or the slave
	if (irq_num & 0x08) {
		slave_mask &= mask; //Cache the fact that we have disabled the IRQ	
		outb(slave_mask, SLAVE_8259_DATA_PORT); //Write the mask to the PIC
	}
	else {
		master_mask &= mask;	
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
}

/* disable_irq
 * DESCRIPTION: disables or masks a specified IRQ line by sending bytes to the approprate PIC(s)
 * INPUTS: irq_num - the IRQ line to alter
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void
disable_irq(uint32_t irq_num)
{
	uint8_t mask = 0x01 << irq_num;

	//NOTE: might need a spinlock here...
	
	//Check if the IRQ is on the master or the slave
	if (irq_num & 0x08) {
		slave_mask |= mask; //Cache the fact that we have disabled the IRQ	
		outb(slave_mask, SLAVE_8259_DATA_PORT); //Write the mask to the PIC
	}
	else {
		master_mask |= mask;	
		outb(master_mask, MASTER_8259_DATA_PORT);
	}

}

/* send_eoi
 * DESCRIPTION: sends an EIO signal to a specified PIC
 * INPUTS: irq_num - the IRQ line to send the byte to
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void
send_eoi(uint32_t irq_num)
{
	//Check if the IRQ is on the master or the slave
	if (irq_num & 0x08) {
		outb(EOI | (irq_num - 8), SLAVE_8259_CMD_PORT);
		outb(EOI | 2, MASTER_8259_CMD_PORT);
	}
	else
		outb(EOI | irq_num, MASTER_8259_CMD_PORT);
}

/* mask_all_irq
 * DESCRIPTION: sets all IRQ lines as masked
 * INPUTS: none
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void 
mask_all_irq(void) {
	outb(MASK_ALL_IRQ, MASTER_8259_DATA_PORT);
	outb(MASK_ALL_IRQ, SLAVE_8259_DATA_PORT);
}

/* restore_all_irq
 * DESCRIPTION: sets all IRQ lines as unmasked
 * INPUTS: none
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void
restore_all_irq(void) {
	outb(master_mask, MASTER_8259_DATA_PORT);
	outb(slave_mask, SLAVE_8259_DATA_PORT);
}

/* send_eoi_all_irq
 * DESCRIPTION: sends an End of interrupt signal for every IRQ line
 * INPUTS: none
 * OUTPUTS: a few bytes
 * RETURN VAL: none
 */
void 
send_eoi_all_irq(void)
{
	int i;
	for(i=0; i < MAX_IRQ; i++)
		send_eoi(i);
}
