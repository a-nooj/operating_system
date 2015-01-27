#include "rtc.h"
#include "lib.h"
#include "idt_entry_handler.h"

/*
* rtc_open
*   DESCRIPTION: Opens the real time clock file descriptor.
*   INPUTS: const uint8_t* filename - unused
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
*/
int32_t rtc_open (int32_t fd) {
	unsigned long flags;

	//Disable interrupts
	cli_and_save(flags);

	//Write rtc rate as 2 Hz
	uint32_t frequency = RTC_DEFAULT;
	rtc_write(NULL, &frequency, 0);

	//Enable interrupts
	restore_flags(flags);
	sti();
	return 0;
}


/*
* rtc_close
*   DESCRIPTION: Closes the real time clock file descriptor.
*   INPUTS: int32_t fd - unused
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: none
*/
int32_t rtc_close (int32_t fd) {
	return 0;
}

/*
* rtc_read
*   DESCRIPTION: Waits for RTC interrupt to occur.
*   INPUTS: int32_t fd - unused
*           void* buf - unused
*           int32_t nbytes - unused
*   OUTPUTS: none
*   RETURN VALUE: 0
*   SIDE EFFECTS: waits until RTC interrupt occurs.
*/
int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes) {
	rtc_interrupted = 0;
	//Wait for interrupt
	while (rtc_interrupted == 0);
		printf("");
	return 0;
}

/*
* rtc_write
*   DESCRIPTION: Sets the RTC periodic interrupt rate.
*   INPUTS: int32_t fd - unused
*           void* buf - pointer to rate
*           int32_t nbytes - unused
*   OUTPUTS: none
*   RETURN VALUE: number of bytes written on success, -1 on invalid frequency.
*   SIDE EFFECTS: sets RTC frequency.
*/
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes) {
	unsigned long flags;
	uint32_t rtc_rate = *((uint32_t*) buf);

	//Disable interrupts
	cli_and_save(flags);

	unsigned char RTC_rate_arg = rate_to_arg(rtc_rate);

	//Check rtc rate is a permitted value
	if (RTC_rate_arg == 0)
	{
		restore_flags(flags);
		sti();
		return -1;
	}
	//Write rtc rate to rtc
	outb(REG_A_MASK_NMI, RTC_ADDR_PORT);	
	unsigned char prev = inb(RTC_DATA_PORT);
	outb(REG_A_MASK_NMI, RTC_ADDR_PORT);	
	outb((prev & 0xF0) | RTC_rate_arg, RTC_DATA_PORT);

	//Enable interrupts
	restore_flags(flags);
	sti();
	return 0;
}

//As stated in discussion, rtc open should just initialize the rtc. 
//So make aure it sets it to the correct frequency. Close does nothing. 
//I'm positive I said this somewhere else on piazza, but for read and write 
//just make an iterative loop that does a read inside each iteration. 
//Then do a write and do the loop again. The loop should go faster or slower based on what 
//you wrote. You can put print statements inside of the loop to see this visually. 


/* 
* rtc_handler
*   DESCRIPTION: Called when rtc interrupt is generated
*   INPUTS: n/a
*   OUTPUTS: calls test_interrupts() function periodically
*   RETURN VALUE: n/a
*   SIDE EFFECTS: n/a
*/
void rtc_handler(void) {
	uint32_t flags;
	cli_and_save(flags);

	//test_interrupts();

	outb(REG_C, RTC_ADDR_PORT);
	inb(RTC_DATA_PORT);

	send_eoi(RTC_IRQ);

	restore_flags(flags);
	sti();
}


/* 
* initialize_rtc
*   DESCRIPTION: Called in kernel.c at OS startup
*   INPUTS: n/a
*   OUTPUTS: sets appropriate rtc flags to initialize rtc interrupts
*	 and enables interrupt request for rtc
*   RETURN VALUE: n/a
*   SIDE EFFECTS: n/a
*/
void initialize_rtc(void) {
	unsigned long flags;
	cli_and_save(flags); //Disable interrupts

	enable_irq(PIC_CASCADE_IRQ);
	enable_irq(RTC_IRQ);

	outb(REG_B_MASK_NMI, RTC_ADDR_PORT);
	unsigned char prev = inb(RTC_DATA_PORT);
	outb(REG_B_MASK_NMI, RTC_ADDR_PORT);
	outb(prev | ENABLE_PERIODIC_INPUT, RTC_DATA_PORT);

	//virtualize rtc
	uint32_t max_rate = 11;
	outb(REG_A_MASK_NMI, RTC_ADDR_PORT);
	prev = inb(RTC_DATA_PORT);
	outb(REG_A_MASK_NMI, RTC_ADDR_PORT);
	outb((prev & WRITE_MASK) | max_rate, RTC_DATA_PORT);


	restore_flags(flags);
}

/*
* rate_to_arg
*   DESCRIPTION: returns appropriate byte for specific rate
*   INPUTS: uint32_t rtc_rate - frequency desired
*   OUTPUTS: none
*   RETURN VALUE: byte corresponding to frequency
*   SIDE EFFECTS: none
*/
unsigned char rate_to_arg(uint32_t rtc_rate){
	switch(rtc_rate) {
		case (2):  return 0x0F;
		case (4):  return 0x0E;
		case (8):  return 0x0D;
		case (16):  return 0x0C;
		case (32):  return 0x0B;
		case (64):  return 0x0A;
		case (128):  return 0x09;
		case (256):  return 0x08;
		case (512):  return 0x07;
		case (1024): return 0x06;
		default: return 0x00;
	}
}
