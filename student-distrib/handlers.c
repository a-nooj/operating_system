/*handler.c - contains functions for exception handling and interrupt handling.*/
/*these functions are all called from an interrupt context, so write them accordingly*/
#include "handlers.h"
#include "rtc.h"

#define KEYBOARD_PORT 0x60
#define KEYBOARD_ENABLE 0xF4
#define KEYBOARD_IRQ 1
#define MOUSE_IRQ 12
#define BREAKCODE 0xA0

#define RTC_IRQ 8
#define RTC_PORT1 0x70
#define RTC_PORT2 0x71
#define RTC_REGC  0x0C

//global var's - for function pointers of irq handlers
//these function pointers and flags are edited by request_irq and free_irq
//might need to impliment spinlocks on these once we have multiple processes?
void (*assigned_kbd_handler)(void) = NULL;
void (*assigned_rtc_handler)(void) = NULL;
void (*assigned_pit_handler)(void) = NULL;
void (*assigned_mouse_handler)(void) = NULL;

extern PCB * ProcessPCBs[MAX_PROCESSES+1]; //reference the process pcbs from elsewhere (process.c)

/* handler functions
 * DESCRIPTION: Each of these is a dedicated function for handling a system exception. Currently, they all act like a BSOD, locking up the machine
 *				Debug info about what was on the stack when the exception was called is printed, as well as what exception occured
 *				unkexcepthandler's job is to catch any exception that we didn't anticipate getting. If it gets called, something needs to be fixed.
 * INPUTS: 5 or 6 arguments, depending on whether the exception creates an error code or not
 *			These inputs are pushed onto the stack when the exception occurs. See page 20 if the
 *			MP3 doc for the stack. 
 *
 *			Argument | Argument | Argument
 *		    contents | (for 5)  | (for 6)
 *        -----------+----------+------------
 *		  Error Code |    N/A   |    1
 *        Return Addr|      1   |    2 
 *        	  CS     |      2   |    3
 *          EFLAGS   |      3   |    4
 *           ESP     |      4   |    5
 *            SS     |      5   |    6
 *
 * OUTPUTS: Currently nothing. These are just BSOD functions that hang and annoy the crap out of the user
 * RETURN VAL: nothing, silly! These are called from an interrupt context, there's nothing to return!
 * NOTES: we will eventually have to edit these functions to kill the user program which caused them. but 
 *			for now, they just do the BSOD thingy.
 */
/***************************************/
/* Begin section for exception handlers*/
/***************************************/
void unkexcepthandler(unsigned long firstarg)
{
	//clear();
	printf("Unknown Exception occured\n");
	printf("with argument1 0x%#x\n", firstarg);
	printf("something wiggy goin on! gotta fix it brah!\n");
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler0(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printk("Div by zero Exception Happened(0)\n");
	printk("ret.addr 0x%#x\n", firstarg);
	printk("CS   0x%#x\n", secondarg);
	printk("EFLAGS    0x%#x\n", thirdarg);
	printk("ESP     0x%#x\n", fourtharg);
	printk("SS        0x%#x\n", fiftharg);
	
	// printf("Div by zero Exception Happened (0)\n");
	// printf("ret.addr 0x%#x\n", firstarg);
	// printf("CS 	     0x%#x\n", secondarg);
	// printf("EFLAGS   0x%#x\n", thirdarg);
	// printf("ESP      0x%#x\n", fourtharg);
	// printf("SS       0x%#x\n", fiftharg);
	//goto http://abstrusegoose.com/440
	//while(1);
	send_signal(DIV_ZERO, current_process);
}
void handler3(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("Breakpoint Exception Happened (3)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler4(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("Overflow Exception Happened (4)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler5(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("BOUND range exceeded Exception Happened (5)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler6(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("Invalid Opcode Exception Happened (6)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler7(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("MathCoprocessor Not Available Exception Happened (7)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler8(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("DoubleFault Exception Happened (8)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler9(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("CoProcessor Segment Overrun Exception Happened (9)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler10(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("Invalid TSS Exception Happened (10)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler11(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("Segment Not Present Exception Happened (11)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler12(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("Stack-Segment Fault Exception Happened (12)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler13(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("General Protection Fault Exception Happened (13)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler14(	unsigned long excep_num, unsigned long err_code)
{

	printk("Page Fault Exception Happened (14)\n");
	// printk("ret.addr 0x%#x\n", 	ProcessPCBs[current_process]->eip);
	// printk("CS   0x%#x\n", 		ProcessPCBs[current_process]->cs);
	// printk("EFLAGS    0x%#x\n", ProcessPCBs[current_process]->eflags);
	// printk("ESP     0x%#x\n",  	ProcessPCBs[current_process]->esp);
	// printk("SS        0x%#x\n",	ProcessPCBs[current_process]->ss);
	printPCB(current_process);
	
	send_signal(SEGFAULT, current_process);
}

void handler16(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("FPU Math-Fault Exception Happened (16)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler17(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg, unsigned long sixtharg)
{
	//clear();
	printf("Alignment Check Exception Happened (17)\n");
	printf("error code 0x%#x\n", firstarg);
	printf("ret.addr   0x%#x\n", secondarg);
	printf("CS 	       0x%#x\n", thirdarg);
	printf("EFLAGS     0x%#x\n", fourtharg);
	printf("ESP        0x%#x\n", fiftharg);
	printf("SS         0x%#x\n", sixtharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler18(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("Machine Check Exception Happened (18)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
void handler19(unsigned long firstarg, unsigned long secondarg, unsigned long thirdarg, unsigned long fourtharg, unsigned long fiftharg)
{
	//clear();
	printf("SIMD Float-Point Exception Exception Happened (19)\n");
	printf("ret.addr 0x%#x\n", firstarg);
	printf("CS 	     0x%#x\n", secondarg);
	printf("EFLAGS   0x%#x\n", thirdarg);
	printf("ESP      0x%#x\n", fourtharg);
	printf("SS       0x%#x\n", fiftharg);
	//while(1);
	send_signal(SEGFAULT, current_process);
}
/*************************************/
/* End section for exception handlers*/
/*************************************/


/******************************/
/*  begin Debugging functions */
/******************************/

/* unkinthandler
 * DESCRIPTION: prints out an error message showing an unknown IRQ line was raised, sends eoi to both PIC's, and returns
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: 
 */
void unkinthandler()
{
	int i;
	
	//clear();
	printf("An Unexpected IRQ line was raised. Returning...\n");

	//since we dont' know where the irq came from, reset both pic's
	for (i = 0; i<16; i++)
		send_eoi(i);
	
	return; // hypothetically, this "return" should take care of all the funkiness associated with getting back from an interrupt

}


/* irq2_handler
 * DESCRIPTION: prints out an error message showing IRQ2 was raised, sends eoi
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: 
 */
void irq2_handler()
{
	printf("                       IRQ2 was raised. Returning...\n");
	send_eoi(2);
	return; // hypothetically, this "return" should take care of all the funkiness associated with getting back from an interrupt
}

/******************************/
/*   End Debugging functions  */
/******************************/



/* kbdhandler
 * DESCRIPTION: called from assembly wrapper when IRQ line for keyboard is asserted. It calls whatever function was set up to handle keyboard int's
 * INPUTS: none
 *
 * OUTPUTS: Currently nothing. These are just BSOD functions that hang and annoy the crap out of the user
 * RETURN VAL: nothing. These are called from an interrupt context, there's nothing to return!
 * NOTES: handles reseting the keyboard and sending EOI, so the user function doesn't have to deal with that.
 */
void kbd_handler()
{
	
	if(assigned_kbd_handler != NULL) //only execute function if it's been assigned.
		assigned_kbd_handler();
	
	outb(KEYBOARD_PORT, KEYBOARD_ENABLE); //reset keyboard - not sure if we actually need to do this?
	send_eoi(KEYBOARD_IRQ); //send EOI byte to the PIC to enable interrupts again

	return;

}



/* rtchandler
 * DESCRIPTION: Called when the RTC timer interrupt fires off. Should call user-requested function
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: nothing. These are called from an interrupt context, there's nothing to return!
 * NOTES:
 */
void rtc_handler()
{
	++rtc_count; // increment interrupt count

	outb(RTC_REGC, RTC_PORT1);	// select register C
	(void)inb(RTC_PORT2);	// just throw away contents	
	
	if(assigned_rtc_handler != NULL) //only execute function if it's been assigned.
		assigned_rtc_handler();

	//Use the PIT and not the RTC for scheduling
	//schedule_tasks();
	
	send_eoi(RTC_IRQ);
	
	return; 
}

/* pit_handler
 * DESCRIPTION: called when the PIT interrupt occurs
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: causes tasks to be scheduled if 
 */
void pit_handler()
{
	//clear();
	if(assigned_pit_handler != NULL) //only execute function if it's been assigned.
		assigned_pit_handler();
	send_eoi(PIT_IRQ);
	return; // hypothetically, this "return" should take care of all the funkiness associated with getting back from an interrupt
}


/* mouse_handler
 * DESCRIPTION: called when the PIT interrupt occurs
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: causes tasks to be scheduled if 
 */
void mouse_handler()
{
	//clear();
	if(assigned_mouse_handler != NULL) //only execute function if it's been assigned.
		assigned_mouse_handler();
	send_eoi(MOUSE_IRQ);
	return; // hypothetically, this "return" should take care of all the funkiness associated with getting back from an interrupt
}


/* request_irq
 * DESCRIPTION: attempts to attach a function "handler" to an IRQ line. Currently only supports attaching functions which return nothing and take
 *				no arguments. Also, functions can only be attached to used IRQ lines. On our OS, these are only 1 and 8.
 * INPUTS: handler - the function pointer to be attached to the IRQ line
 *		   IRQline - which IRQ line to attach the input handler function to.
 * OUTPUTS: changes global variable associated with handler
 * RETURN VAL: 0 on successful assignment, -1 if assignment failed (bad IRQ input number)
 * NOTES: Currently allows for overwrites on the irq functions - process does not have to free the line before another one is installed
 * 
 */
int request_irq( void (*handler)(void), unsigned int IRQline)
{
	unsigned long flags;
	
	//Disable interrupts
	cli_and_save(flags);
	
	//set IRQ handler for KBD
	if(IRQline == KEYBOARD_IRQ)
	{
		assigned_kbd_handler = handler;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	//set IRQ handler for RTC
	else if(IRQline == RTC_IRQ)
	{
		assigned_rtc_handler = handler;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	//set IRQ handler for PIT
	else if(IRQline == PIT_IRQ)
	{
		assigned_pit_handler = handler;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	//set IRQ handler for Mouse
	else if(IRQline == MOUSE_IRQ)
	{
		assigned_mouse_handler = handler;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	
	//else, return an error
	else
	{
		//enable interrupts
		printf("ERROR: tried to assign handler invalid IRQ line %d", IRQline);
		restore_flags(flags);
		return -1;
	}
}
/* free_irq
 * DESCRIPTION: detaches any function associated with an IRQ line by setting the function pointer associated with that IRQ to NULL
 * INPUTS: IRQline - the line to free up.
 * OUTPUTS: changes global variable associated with handler
 * RETURN VAL: 0 on successful free, -1 if free failed (bad IRQ input number)
 * 
 */
int free_irq(unsigned int IRQline)
{
	unsigned long flags;
	
	//Disable interrupts
	cli_and_save(flags);
	
	//unset IRQ handler for KBD
	if(IRQline == KEYBOARD_IRQ)
	{
		assigned_kbd_handler = NULL;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	//unset IRQ handler for RTC
	else if(IRQline == RTC_IRQ)
	{
		assigned_rtc_handler = NULL;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	else if(IRQline == PIT_IRQ)
	{
		assigned_pit_handler = NULL;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	else if(IRQline == MOUSE_IRQ)
	{
		assigned_mouse_handler = NULL;
		//enable interrupts
		restore_flags(flags);
		return 0;
	}
	//else, return an error
	else
	{
		//enable interrupts
		restore_flags(flags);
		return -1;
	}
}
