/*rtc c - Includes all functions associated with the rtc initalization and rtc driver*/
#include "rtc.h"
#include "x86_desc.h"
#include "lib.h"
#include "debug.h" 
#include "i8259.h"
#include "handlers.h"
#include "process.h"

#define MAX_PROCESSES 6

volatile int32_t rtc_is_open [MAX_PROCESSES + 1]; // zero-initialized automatically
volatile uint32_t rtc_count; // keeps count of how many RTC interrupts have occurred
static int32_t virtual_RTC_rate [MAX_PROCESSES + 1];
/* rtc_init
 * DESCRIPTION: Performs all actions required to initalize rtc
 * INPUTS: 
 *
 * OUTPUTS: 
 * RETURN VAL: 
 * NOTES: taken from http://wiki.osdev.org/RTC
 */
void rtc_init(void)
{
	unsigned long flags;
	
	enable_irq(SLAVE_IRQ); //Slave PIC enabled
	enable_irq(RTC_IRQ); //RTC interrupt enabled
	
	//Disable interrupts
	cli_and_save(flags);

	outb(REG_A_MASK_NMI, RTC_REG_PORT);	// select Status Register A, and disable NMI (by setting the 0x80 bit)
	outb(RTC_MAX_RATE_DATA, RTC_DATA_PORT);	// Set Rate Selector to the max with IRQs every 976 us	
	//outb(RTC_DEFAULT_RATE, RTC_DATA_PORT);	// Set Rate Selector to 1111 corresponding to IRQs 500ms	

	outb(REG_B_MASK_NMI, RTC_REG_PORT);	// select Status Register B, and disable NMI (by setting the 0x80 bit)
	outb(PERIODIC_INPUT_ENABLE, RTC_DATA_PORT);	// Enable Periodic input

	
	//enable interrupts
	restore_flags(flags);

	return;

}


/*
 * rtc_open
 *   DESCRIPTION: Opens the real time clock file descriptor.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 if RTC is already open.
 *   SIDE EFFECTS: sets rtc_is_open to 1 on success.
 */
int32_t rtc_open(void) {
	int32_t retval;
	uint32_t flags;
	
	cli_and_save(flags);
	if (rtc_is_open[current_process]) {
		retval = -1;
	} else {
		rtc_is_open [current_process] = 1;
		uint32_t default_freq = 2;
		rtc_write(NULL, &default_freq, 0);
		retval = 0;
	}
	restore_flags(flags);

	return retval;
}


/*
 * rtc_close
 *   DESCRIPTION: Closes the real time clock file descriptor.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: sets rtc_is_open to 0.
 */
int32_t rtc_close(void) {
	rtc_is_open [current_process] = 0;
	return 0;
}


/*
 * rtc_read
 *   DESCRIPTION: Waits for an RTC interrupt to occur.
 *   INPUTS: file_desc -- unused
 *           buf -- unused
 *           nbytes -- unused
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: blocks until an RTC interrupt occurs.
 */
int32_t rtc_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	
	/**** Physical RTC***/
	// if (rtc_is_open [current_process]){
		// rtc_interrupt_ocurred = 0;
		// while (rtc_interrupt_ocurred == 0); //wait on rtc_interrupt
		// return 0;
	// }
	// else {
		// printk("\nERROR: Tried to read without opening rtc!\n");
		// return -1;
	// }
	
	/**** Virtualized RTC***/
	// T = 1/f (f = rate)
	// T_virt = num_interrupts * T_max_rate
	// num_interrupts = T_virt / T_max_rate
	// 				  = f_max_rate  / f_virt
	//So we can wait for num_interrupts and it will be the same
	uint32_t end = rtc_count + RTC_MAX_RATE / virtual_RTC_rate[current_process];
	// if (rtc_count % (RTC_MAX_RATE * 10))
		// send_signal(current_process);
	
	sti();
	while (rtc_count < end);
	return 0;
}


/*
 * rtc_write
 *   DESCRIPTION: Sets the RTC periodic interrupt frequency.
 *   INPUTS: file_desc -- unused
 *           buf -- the new frequency
 *           nbytes -- unused
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on invalid frequency.
 *   SIDE EFFECTS: sets the RTC frequency.
 */
int32_t rtc_write(file_desc_t* file_desc, const void* buf, int32_t nbytes) {
	uint32_t rate = *((uint32_t*) buf);
	uint32_t flags;
	uint32_t num_bytes_written = 0;
	// unsigned char prev;
	// unsigned char RTC_byte;
	
	//Disable interrupts
	cli_and_save(flags);
			
	//Check min and max
	if (rate < RTC_MIN_RATE || rate > RTC_MAX_RATE)
		return -1;
	
	//Test power of two
	if ( ( rate & (rate - 1) ) != 0  )
		return -1;
	
	/**** Physical RTC***/
	// RTC_byte = RTC_rate_to_byte(rate);
	// //Get the previous value of A
	// outb(REG_A_MASK_NMI, RTC_REG_PORT);	
	// prev = inb(RTC_DATA_PORT);
	// num_bytes_written++;
	// //Write the new rate
	// outb(REG_A_MASK_NMI, RTC_REG_PORT);	
	// outb((prev & 0xF0) | RTC_byte, RTC_DATA_PORT);
	// num_bytes_written += 2;
	
	/**** Virtualized RTC***/
	virtual_RTC_rate[current_process] = rate;
	
	//enable interrupts
	restore_flags(flags);
	
	return num_bytes_written;
}

/*
 * RTC_rate_to_byte
 *   DESCRIPTION: changes a given RTC rate tot eh approprate byte to send.
 *   INPUTS: rate - what rate are we selecting?
 *   OUTPUTS: none
 *   RETURN VALUE: the byte to be sent
 *   SIDE EFFECTS: fries and a drink. 
 */
unsigned char RTC_rate_to_byte(uint32_t rate){
	switch(rate) {
		case (1 << 1):  return 0x0F;
		case (1 << 2):  return 0x0E;
		case (1 << 3):  return 0x0D;
		case (1 << 4):  return 0x0C;
		case (1 << 5):  return 0x0B;
		case (1 << 6):  return 0x0A;
		case (1 << 7):  return 0x09;
		case (1 << 8):  return 0x08;
		case (1 << 9):  return 0x07;
		case (1 << 10): return 0x06;
		default: return 0x0F;
	}
}


