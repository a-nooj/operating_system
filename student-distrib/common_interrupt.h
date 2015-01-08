/* common_interrupt.h - contains assembly wrappers for interrupts and system calls.
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _COMMON_INTERRUPT_H
#define _COMMON_INTERRUPT_H

#include "handlers.h"
#include "types.h"
// #include "process.h"
// #include "signal.h"

#define KERNEL_ADDR 0x400000
#define FULLPAGE 	0x400000
#define STACKSIZE   8192

#define PROCESS_1_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*1))
#define PROCESS_2_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*2))
#define PROCESS_3_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*3))
#define PROCESS_4_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*4))
#define PROCESS_5_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*5))
#define PROCESS_6_PCB_START (PCB*)(KERNEL_ADDR+FULLPAGE-(STACKSIZE*6))


//these consttants should be numerically equal to the two right above (PROCESS_X_PCB_START)
//these are used in assembly insead of the ones above.
#define P1PCBADDR 0x7FE000 
#define P2PCBADDR 0x7FC000
#define P3PCBADDR 0x7FA000 
#define P4PCBADDR 0x7F8000
#define P5PCBADDR 0x7F6000 
#define P6PCBADDR 0x7F4000



extern void pit_interrupt(void);
extern void mouse_interrupt(void);
extern void kbd_interrupt(void);
extern void irq2_interrupt(void);
extern void rtc_interrupt(void);
extern void unkwn_interrupt(void);
extern void syscallwrapper(void);
extern void handler14_linkage(void);


#endif
