/* user_mode.c - Holds function to execute user-level code
 * with context switching
 */

#include "usermode.h"

/*
 * context_switch
 *   DESCRIPTION: Performs a context switch to ring 3.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Starts running executable in user.
 */
void context_switch(unsigned int entry)
{
   // Push registers on stack for IRET to use
	asm volatile("						\n\t"
			
			"							\n\t"
			//"cli						\n\t"	// disable interrupts
			"pushf						\n\t"	// get flags
			"popl %%ebx					\n\t"
			"movw %1, %%ax				\n\t"	// set segment selectors to user-mode data selector FIX IF NECESSARY CHANGE TO %1
			"movw %%ax, %%ds			\n\t"	// set DS register to point to USER_DS
			"movw %%ax, %%es			\n\t"	// rest of segments reference data segment
			"movw %%ax, %%fs			\n\t"
			"movw %%ax, %%gs			\n\t"
			"							\n\t"
			"movl %%esp, %%eax			\n\t"	// save stack pointer from before we changed the stack
			"pushl %1					\n\t"	// push SS
			"pushl %2					\n\t"	// push 132 MB (bottom of page in virtual memory)
			"pushl %%ebx				\n\t"	// push flags
												// Pushed EBX instead of flags so IF = 0 when pushed out.
												// Moves and pushes shouldn't change flags, right?
			"							\n\t"
			"pushl %0					\n\t"	// push CS
			"pushl %3					\n\t"	// push executable EIP from loader
												// DO THIS!!!!
			"							\n\t"
			"pushl %%eax				\n\t"
			"call modify_tss;			\n\t"	// modify TSS to set values of SS0 and ESP0
	//		"addl $4, %%esp				\n\t"
	//		"							\n\t"
	//		"iret						\n\t"	// start executing user code
			//"halt_ret:					\n\t"	// from discussion, not sure if want to use
			"							\n\t"
			:									// outputs - none
			: "g"(USER_CS), "g"(USER_DS), "g"(USER_STACK_PTR), "g"(entry) // inputs
			: "memory", "%eax", "%ebx"					// clobbered
     );
	 asm volatile("addl $4, %%esp":::"memory");
	 asm volatile("movl $0x080482D0, (%%esp)"::);
	 asm volatile("iret");
} 

/*
 * modify_tss
 *   DESCRIPTION: Helper function to modify TSS before calling IRET.
 *   INPUTS: old ESP value
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Changes ESP0 and SS0 in the TSS.
 */
void modify_tss(int32_t old_esp)
{
	tss.esp0 = old_esp;		// IRET pops everything pushed, so point to stack before
	tss.ss0 = KERNEL_DS;	// ss0 holds kernel's data segment
	
	//return;
}
