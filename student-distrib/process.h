/* process.h - process related prototypes */

#ifndef _PROCESS_H
#define _PROCESS_H

#include "types.h"
#include "common_interrupt.h"
#include "process.h"
#include "paging.h"
#include "x86_desc.h"
#include "lib.h"
#include "file_descriptor.h"
#include "terminal.h"
#include "signal.h"
#include "soundblaster.h"

#define RTC_DEFAULT_RATE 0x2F //2 Hz

#define NUM_SIGS 5

#define PROCESS_1_PHYS_ADDR 0x0800000
#define PROCESS_2_PHYS_ADDR 0x0C00000
#define PROCESS_3_PHYS_ADDR 0x1000000
#define PROCESS_4_PHYS_ADDR 0x1400000
#define PROCESS_5_PHYS_ADDR 0x1800000
#define PROCESS_6_PHYS_ADDR 0x1C00000


#define PROCESS_1_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*0-4)
#define PROCESS_2_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*1-4)
#define PROCESS_3_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*2-4)
#define PROCESS_4_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*3-4)
#define PROCESS_5_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*4-4)
#define PROCESS_6_KERNEL_STACK (KERNEL_ADDR+FULLPAGE-STACKSIZE*5-4)
                        
#define PROCESS_1_VIRT_ADDR 0x8000000
#define PROCESS_2_VIRT_ADDR 0x8000000
#define PROCESS_3_VIRT_ADDR 0x8000000
#define PROCESS_4_VIRT_ADDR 0x8000000
#define PROCESS_5_VIRT_ADDR 0x8000000
#define PROCESS_6_VIRT_ADDR 0x8000000

#define PROCESS_VIRT_ADDR 0x8000000

#define PROCESS_1_STACK_BASE (PROCESS_1_VIRT_ADDR + FULLPAGE-4) //grow stacks from the bottom of the pages for the user processes
#define PROCESS_2_STACK_BASE (PROCESS_2_VIRT_ADDR + FULLPAGE-4) 
#define PROCESS_3_STACK_BASE (PROCESS_3_VIRT_ADDR + FULLPAGE-4)
#define PROCESS_4_STACK_BASE (PROCESS_4_VIRT_ADDR + FULLPAGE-4)
#define PROCESS_5_STACK_BASE (PROCESS_5_VIRT_ADDR + FULLPAGE-4)
#define PROCESS_6_STACK_BASE (PROCESS_6_VIRT_ADDR + FULLPAGE-4)

#define PROCESS_OUTPUT_PAGE 0x0000000

#define VRAM_VIRT_ADDR 0xb8000

#define MAGIC0 0x7F
#define MAGIC1 0x45
#define MAGIC2 0x4C
#define MAGIC3 0x46

#define OFFSET 0x00048000

#define DEFAULT_EFLAGS 0x00000246 //PF, IF, ZF all set. IF is most important to be set...

#define MAX_OPEN_FILES 8

#define MAX_PROCESSES 6

#define SUFFICENTLY_LARGE_NUMBER 50 //maximum length of an executable name

//structure for Process Control Block data
//this structure goes at the top of the block
//the stack starts at the bottom
typedef struct {
/*0*/	uint32_t eax; //places to save all the registers for the process state
/*4*/	uint32_t ebx; //not sure if these are all needed, or if we need to save other registers?
/*8*/	uint32_t ecx; //registers should be at the top so that it's easy to index into it from assembly code
/*12*/	uint32_t edx; //DO NOT CHANGE THIS ORDER WITHOUT ALSO CHANGING THE HANDLER IN common_interrupt.S
/*16*/	uint32_t ebp;
/*20*/	uint32_t esp;
/*24*/	uint32_t eip;	
/*28*/	uint32_t esi;
/*32*/	uint32_t edi;
/*36*/	uint32_t kernel_esp;
/*40*/	uint16_t ds;
/*42*/	uint16_t cs;
/*44*/	uint16_t ss;
/*48*/	uint32_t eflags;
	
	pde_t PageDirectory; //page directory for process
	file_desc_t process_file_desc_table[MAX_OPEN_FILES]; //file descriptors array
	
	uint32_t terminal_num; //says which terminal this process was started on
	
	uint32_t parent_process; //says which process started this process
	uint32_t child_process; //says which process has been started by this process

	uint32_t RTCsetting; //process's setting for the RTC
	
	char arguments[SUFFICENTLY_LARGE_NUMBER*5];
	int lengthofargs;
	
	//Signal handling info
	void (*handler [NUM_SIGS]); //Handler array
	uint8_t pending [NUM_SIGS]; //pending signals for each process	
	uint8_t masked [NUM_SIGS]; //mask for each process
	
	
} PCB;

//global variables

//integer representing the currently executing process
volatile int current_process;
//array showing which processes are currently in use
extern unsigned char PIDinUse[MAX_PROCESSES+1];

/* Loads and executes a program */
extern int32_t execute(const int8_t* command);

/* Terminates a process */
// changing return type to void since it's not supposed to return
extern void halt(uint16_t status);

/* Gets the arguments to a process */
extern int32_t getargs(int8_t* buf, int32_t nbytes);

/* Maps video memory into user-space */
// I'm majorly confused about this system call
extern int32_t vidmap(uint8_t** screen_start);

/*starts the inital shell from the OS boot*/
extern int32_t start_initial_shells();

/*moves a currently-executing process out of memory and puts a new one in*/
extern int32_t switch_process(int PIDout, int PIDin);

//Halts a particular process and jumps to its parent
void halt_proc(uint16_t proc_id, uint16_t status);

//Checks to see if process PID has a child process
extern int has_child_process(int pid);

extern void printPCB (uint32_t pid);

#endif