#include "process.h"
#include "paging.h"
#include "terminal.h"

extern terminal_data terms[MAX_NUM_TERMS];

/* process.c - process related functions */
//global variables

//variables are not actually created for the PCB's, 
//we just define a struct for indexing purposes
//and then assume that the memory locations we define 
//will be free and not overwritten by anything else.
//THE COMPILER HAS NO KNOWLEDGE THAT WE ARE STORING DATA AT THESE LOCATIONS!!!!
//VERY HACKY, but the way it has to be done

//the array referenced above
//max 12 (??) processes - three terminals, each with three shells plus a program on top of the shells...? I think?
PCB * ProcessPCBs[MAX_PROCESSES+1] = {
					   NULL, 			    //inital kernel has no PCB
					   PROCESS_1_PCB_START,
					   PROCESS_2_PCB_START,
					   PROCESS_3_PCB_START,
					   PROCESS_4_PCB_START,
					   PROCESS_5_PCB_START,
					   PROCESS_6_PCB_START};
						//need to add in the rest of the pcb pointers here for checkpoints 3.4 and 3.5 for all possible processes					   


//integer representing the currently executing process
extern volatile int current_process;

//array of process physical address pointers
uint32_t ProcessPhysAddr[MAX_PROCESSES+1] = {
						NULL,
						PROCESS_1_PHYS_ADDR,
						PROCESS_2_PHYS_ADDR,
						PROCESS_3_PHYS_ADDR,
						PROCESS_4_PHYS_ADDR,
						PROCESS_5_PHYS_ADDR,
						PROCESS_6_PHYS_ADDR};

uint32_t KernelStackAddr[MAX_PROCESSES+1] = {
						NULL,
						PROCESS_1_KERNEL_STACK,
						PROCESS_2_KERNEL_STACK,
						PROCESS_3_KERNEL_STACK,
						PROCESS_4_KERNEL_STACK,
						PROCESS_5_KERNEL_STACK,
						PROCESS_6_KERNEL_STACK};

uint32_t UserStackAddr[MAX_PROCESSES+1] = {
						NULL,
						PROCESS_1_STACK_BASE,
						PROCESS_2_STACK_BASE,
						PROCESS_3_STACK_BASE,
						PROCESS_4_STACK_BASE,
						PROCESS_5_STACK_BASE,
						PROCESS_6_STACK_BASE};
						
uint32_t ProcessOutputPage[MAX_PROCESSES+1] = {
						NULL,
						PROCESS_1_STACK_BASE,
						PROCESS_2_STACK_BASE,
						PROCESS_3_STACK_BASE,
						PROCESS_4_STACK_BASE,
						PROCESS_5_STACK_BASE,
						PROCESS_6_STACK_BASE};
						
//array of chars - 0 if that process ID is avaliable, 1 if it is in use.
unsigned char PIDinUse[MAX_PROCESSES+1];

//Local function
void printPCB(uint32_t pid);


/*
 * execute
 *   DESCRIPTION: Loads and executes a program.
 *   INPUTS: command -- the command to execute
 *   OUTPUTS: none.
 *   RETURN VALUE: program status code on success,
 *                 256 if program dies by an exception,
 *                 -1 on failure.
 *   SIDE EFFECTS: launches a new process.
 */
int32_t execute(const int8_t* command) { 

	int i,j; //counter variables
	int newPID;
	unsigned char executable_header[40];	
	int32_t offset = 0;
	int32_t csarg = USER_CS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int32_t dsarg = USER_DS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int32_t filesize = 0;
	int8_t executable_name[SUFFICENTLY_LARGE_NUMBER];
	
	//find the next available process ID number to use
	cli();
	send_eoi_all_irq();

	
	newPID = -1;
	for(i = 0; i < MAX_PROCESSES+1; i++)
	{
		if(PIDinUse[i] != 1)
		{
			newPID = i;
			i = MAX_PROCESSES+2;//use this to break out of the for loop once we've found a free PID
		}
	} 
	//handle case where we already have all processes in use
	if(newPID == -1)
	{
		write(1,"Cannot Execute, not enough system resources to add another process\n", 68);
		return -1;
	}
	
	//Zero out all the data in the PCB
	ProcessPCBs[newPID]->eax = 0; 
	ProcessPCBs[newPID]->ebx = 0; 
	ProcessPCBs[newPID]->ecx = 0; 
	ProcessPCBs[newPID]->edx = 0; 
	ProcessPCBs[newPID]->ebp = 0;
	ProcessPCBs[newPID]->esp = 0;
	ProcessPCBs[newPID]->eip = 0;	
	ProcessPCBs[newPID]->esi = 0;
	ProcessPCBs[newPID]->edi = 0;
	ProcessPCBs[newPID]->kernel_esp = 0;
	ProcessPCBs[newPID]->ds = 0;
	ProcessPCBs[newPID]->cs = 0;
	ProcessPCBs[newPID]->ss = 0;
	ProcessPCBs[newPID]->eflags = DEFAULT_EFLAGS;
	ProcessPCBs[newPID]->terminal_num = 0; 
	ProcessPCBs[newPID]->parent_process = 0; 
	ProcessPCBs[newPID]->RTCsetting = RTC_DEFAULT_RATE; 	
	
	//clear out two strings
	for(i = 0; i < SUFFICENTLY_LARGE_NUMBER; i++)
		executable_name[i] = 0x00;
	for(i = 0; i < SUFFICENTLY_LARGE_NUMBER*5; i++)
		ProcessPCBs[newPID]->arguments[i] = 0x00;
	
	//extract off the executable name, which is everything in the command before the first space 
	for(i = 0; command[i] != ' ' && command[i] != NULL; i++)
		executable_name[i] = command[i];
	//strip leading spaces
	for(;command[i] == ' ' && command[i] != NULL; i++);
	//put arguments into pcb for later use by getargs system call
	for(j = 0;command[i] != NULL; i++, j++)
		ProcessPCBs[newPID]->arguments[j] = command[i];

	ProcessPCBs[newPID]->lengthofargs= j-1; //minus 1 because j gets incrimented one extra time within the for loop;
	
	//note - until validity of new program is verified and the file is copied to memory, the file descriptor table from the function that called execute
	//is used
	int32_t fd = open(executable_name); //open the file for validy checking (look at header)
	
	if(fd == -1) //if file could not be opened, return error
		return -1;
	
	read(fd, executable_header, 40); //read in the header
	
	//check magic number, return error if it's not correct
	if((executable_header[0] != MAGIC0) || (executable_header[1] != MAGIC1) || (executable_header[2] != MAGIC2) || (executable_header[3] != MAGIC3))
	{
		close(fd);
		return -1;
	}
	//since we know that we will execute this, set the PID as in use
	PIDinUse[newPID] = 1;
	
	//Clear any signal info for the new proces
	clear_signals(newPID);
	
	//We are excuting this on the current terminal
	terms[current_terminal].process_on_this_term = newPID;
	
	//read in offset
	offset = (executable_header[27] << 24) + (executable_header[26] << 16) + (executable_header[25] << 8) + (executable_header[24] << 0);
	
	close(fd); //reset to begining of the program by simply reopening the file
	fd = open(executable_name);
	
	//assume we keep only one Page Directory, but re-map the virtual addresses for user space on each context switch
	map_process_page(ProcessPhysAddr[newPID]); //map page for new process
	
	filesize = read(fd, (void *)(PROCESS_VIRT_ADDR+OFFSET), FULLPAGE); //read the entire program (minus the header) into the process1 page with offset
	close(fd); //close out the file descriptor, now unneeded.
	
	//fill in PCB for new program. 
	//register values are filled in by the assembly wrapper before this C code gets called
	ProcessPCBs[newPID]->parent_process = current_process;
	ProcessPCBs[newPID]->terminal_num = current_terminal; //set to be active in current terminal
	for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table
	{
		ProcessPCBs[newPID]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
		ProcessPCBs[newPID]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
	}
	ProcessPCBs[newPID]->process_file_desc_table[0].flags |= DESC_IN_USE; //Fill in FD table properly - open stdin/stdio
	ProcessPCBs[newPID]->process_file_desc_table[0].file_ops = &file_ops[FILE_TYPE_STDIN];
	ProcessPCBs[newPID]->process_file_desc_table[1].flags |= DESC_IN_USE;
	ProcessPCBs[newPID]->process_file_desc_table[1].file_ops = &file_ops[FILE_TYPE_STDOUT];
	file_desc_table = ProcessPCBs[newPID]->process_file_desc_table; //set this file descriptor table as the active one (file_desc_table declared in file_descriptor.h)
	
	//set tss for new kernel stack
	tss.ss0 = KERNEL_DS; //segment where the kernel stack is located
	tss.esp0 = KernelStackAddr[newPID]; //kernel stack base
	
	current_process = newPID; //set global variable to show the new process is running 
						 //this will eventually be set only by the scheduler.
	
	//printPCB(newPID);
	
	//execute assembly to switch to user mode - this chunk of assembly should never return
	asm volatile(
		
		//set up stack for iret
		"pushl %1   \n\t" 			//push DS entry
		"pushl %3   \n\t" 			//push new esp argument
		"pushl %5  	\n\t" 			//push current eflags to new process
		"pushl %0   \n\t" 			//push CS entry
		"pushl %2   \n\t" 			//push entry point
		
		//do some miscelanous non-stack things
		"movw %4, %%ax   \n\t"
		"movw %%ax, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
		"movl %3, %%ebp   \n\t" //set up new stack base pointer
		
		//jumpt to the user space program
		"sti	\n\t"
		"iret   \n\t"	   			//execute the new program
		:
		:"g" (csarg), "g" (dsarg), "g" (offset), "g" (UserStackAddr[current_process]), "g" (USER_DS), "g" (DEFAULT_EFLAGS) 
		:"memory", "eax", "esp"
		);
	
	
	//we should never hit this point, so return minus one cuz something wiggy happened!
	clear();
	printf("\nyou are returning from the execute system call!!!\n This is weird!!!\n look at process.c, execute()\n\n");
	return -1;
	

}

//Status needs to be accessible even after the page table has changed dealing with the weird 
static uint32_t exit_status;

/*
 * halt
 *   DESCRIPTION: Terminates a process.
 *   INPUTS: status -- the status to be returned to the caller.
 *   OUTPUTS: none.
 *   RETURN VALUE: none.
 *   SIDE EFFECTS: returns control back to the caller process.
 */
void halt(uint16_t status) {
	cli();
	char statusascii[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	int32_t csarg = USER_CS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int32_t dsarg = USER_DS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int i = 0; //counter variable

	if(current_process == 1 || current_process == 2 || current_process == 3)
	{
		//a halt call from the inital shell should be interpreted as "shutdown the machine"
		//do any cleanup required, and then spin nicely until the user actually exits the VM
		
		
		//print shutdown message
		//clear();
		write(1, "\n\nTerminal Shutdown with status = ", strlen("\n\nSystem Shutdown with status = "));
		itoa(status, statusascii, 10);
		write(1, statusascii, strlen(statusascii));
		sti(); //make sure interrupts are enabled so other processes can still run
		/* Spin (nicely, so we don't chew up cycles) */
		asm volatile(".1: hlt; jmp .1;");
	}
	else
	{
		// write(1, "status = ", strlen("ststus = "));
		// itoa(status, statusascii, 10);
		// write(1, statusascii, strlen(statusascii));
		// write(1, "\n", strlen("\n"));
		// exit_status = status;
		
		for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table for the halting function (for security purposes)
		{
			ProcessPCBs[current_process]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
			ProcessPCBs[current_process]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
		}
		
		file_desc_table = ProcessPCBs[ProcessPCBs[current_process]->parent_process]->process_file_desc_table; //set the old file descriptor table as the active one (file_desc_table declared in file_descriptor.h)
		
		send_eoi_all_irq();
		clear_signals(current_process);
		//close the RTC for this process
		rtc_close();
		
		//close the soundcard in case the process forgot. 
		AdLib_close();
	
		//set tss for old kernel stack
		tss.ss0 = KERNEL_DS; //segment where the kernel stack is located
		//tss.esp0 = ProcessPCBs[ProcessPCBs[current_process]->parent_process]->kernel_esp; //old esp from last kernel stack, from before execute happened
		tss.esp0 = KernelStackAddr[ProcessPCBs[current_process]->parent_process]; //kernel stack base
		
		PIDinUse[current_process] = 0; //clear entry, as this process is no longer taking up memory space
		current_process = ProcessPCBs[current_process]->parent_process; //set global variable to show the old process is now running
																		//this will eventually be set only by the scheduler.		
		terms[ProcessPCBs[current_process]->terminal_num].process_on_this_term = current_process;
		
		map_process_page(ProcessPhysAddr[current_process]);

		//printf("flags=0x%x", ProcessPCBs[current_process]->eflags);
		

		asm volatile(
		"movl 	%6, %%eax		\n\t" 			//set return value into EAX
		
		//set up stack for iret
		"pushl %1   \n\t" 			//push DS entry
		"pushl %3   \n\t" 			//push previous esp argument
		
		"xor %%edi, %%edi			\n\t"		
		"orl %8, %%edi  	\n\t" 			//push current eflags to new process
		"pushl %%edi			\n\t"
		// "pushf \n"
		
		"pushl %0   \n\t" 			//push CS entry
		"pushl %2   \n\t" 			//push code return point


		
		//restore registers from previous process
		"movl	%7, %%edi \n\t" 				// set destination register to the memory location of the old process PCB
												// EAX can be used as a working register since it will be overwritten at the end anyway
		"movl	4(%%edi), %%ebx \n\t"			// restore EBX
		"movl	8(%%edi), %%ecx  \n\t"			// restore ECX
		"movl	12(%%edi), %%edx  \n\t"			// restore EDX
		"movl	16(%%edi), %%ebp  \n\t"			// restore EBP
												// ESP and EIP restored below
		"movl	28(%%edi), %%esi  \n\t"			// restore ESI								
		"movl	32(%%edi), %%edi \n\t"			// move the old EDI value to EDI. As this overwrites our working pointer, it should be done last.

		//do some miscelanous non-stack things
		"movw %4, %%di   \n\t"  //for some reason, can't store directly to %ds from memory, use edi as temporary
		"movw %%di, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
		//"movl %5, %%ebp   \n\t" //set up old stack base pointer
		//AJ: We already restored ebp. Restoring it againg causes a pagefault	
		
		"sti 	\n\t"
		//jump to the user space program
		"iret   \n\t"	   			//jump back to the old program

		:
		//: "m" (csarg), "m" (dsarg), "m" (Process1PCB->eip), "m" (Process1PCB->esp), "e" (USER_DS), "m" (Process1PCB->ebp), "X" ((unsigned long)status), "r" (Process1PCB)
		: "g" (csarg), "g" (dsarg), "g" (ProcessPCBs[current_process]->eip), "g" (ProcessPCBs[current_process]->esp), "g" (USER_DS), "g" (ProcessPCBs[current_process]->ebp), "g" (exit_status), "g" (ProcessPCBs[current_process]), "g" (ProcessPCBs[current_process]->eflags)
		:"memory", "eax", "ebx", "ecx", "edx", "edi", "ebp"
		);
	
	
	}

	//we should never hit this point, so return minus one cuz something wiggy happened!
	clear();
	printf("\nyou are returning from the halt system call!!!\n This is weird!!!\n look at process.c, halt()\n\n");

}


/*
 * getargs
 *   DESCRIPTION: Gets the arguments to a process.
 *   INPUTS: nbytes -- the number of bytes in the buffer.
 *   OUTPUTS: buf -- a buffer in which to write the arguments.
 *   RETURN VALUE: 0 on success, -1 if buffer is too small.
 *   SIDE EFFECTS: none.
 */
int32_t getargs(int8_t* buf, int32_t nbytes) {
	
	//check for valid buffer
	if((buf < (int8_t *)PROCESS_VIRT_ADDR) || (buf > (int8_t *)(PROCESS_VIRT_ADDR+FULLPAGE)))
		return -1;
	
	//check to make sure that we can fit the arguments string into the buffer
	if(nbytes < ProcessPCBs[current_process]->lengthofargs)
		return -1;
		
	//copy argument to user program
	
	strncpy(buf, ProcessPCBs[current_process]->arguments, nbytes);
	
	return 0;
}


/*
 * vidmap
 *   DESCRIPTION: Maps video memory into user-space.
 *   INPUTS: none.
 *   OUTPUTS: screen_start -- the virtual address of the mapping.
 *   RETURN VALUE: 0 on success, -1 for invalmakid screen_start.
 *   SIDE EFFECTS: changes virtual memory mappings.
 */
int32_t vidmap(uint8_t** screen_start) {
	
	//check for valid double pointer
	if((screen_start < (uint8_t **)PROCESS_VIRT_ADDR) || (screen_start > (uint8_t **)(PROCESS_VIRT_ADDR+FULLPAGE)))
		return -1;
	
	*screen_start = (uint8_t*) USER_VGA_ADDR;
	return 0;
}


/*
 * start_initial_shells
 *   DESCRIPTION: starts up the inital shell program in all three terminals. called from kernel.c, should never return
 *   INPUTS: none.
 *   OUTPUTS: none
 *   RETURN VALUE: 0 always
 *   SIDE EFFECTS: 
 */
int32_t start_initial_shells()
{
	int i; //counter variable
	unsigned char executable_header[40];	
	int32_t offset = 0;
	int32_t csarg = USER_CS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int32_t dsarg = USER_DS & 0x0000FFFF; //make sure argument is passed with proper bitmasking
	int32_t filesize = 0;
	
	PIDinUse[0] = 1; //mark inital kernel as in use
	PIDinUse[1] = 1; // first three shells use the first three PID's
	PIDinUse[2] = 1;
	PIDinUse[3] = 1;
	//initalize the rest of the PID's as unused
	for(i = 4; i <= MAX_PROCESSES; i++)
		PIDinUse[i] = 0;

		
	//fill in PCB for new shell in terminal 0. We can do this now because there is no reason the shell should not launch/execute
	//we need to do it now so that we have a file descriptor structure to work with to open the inital shell
	//we do the other PCB's later after we get the inital EIP from the executable's header
	ProcessPCBs[1]->parent_process = 0; //the shell was started at boot - it has no parent
	ProcessPCBs[1]->terminal_num = 0; //inital shell runs on terminal 0
	for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table
	{
		ProcessPCBs[1]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
		ProcessPCBs[1]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
	}
	ProcessPCBs[1]->process_file_desc_table[0].flags |= DESC_IN_USE; //Fill in FD table properly - open stdin/stdio
	ProcessPCBs[1]->process_file_desc_table[0].file_ops = &file_ops[FILE_TYPE_STDIN];
	ProcessPCBs[1]->process_file_desc_table[1].flags |= DESC_IN_USE;
	ProcessPCBs[1]->process_file_desc_table[1].file_ops = &file_ops[FILE_TYPE_STDOUT];
	ProcessPCBs[1]->lengthofargs = 1;
	ProcessPCBs[1]->lengthofargs = 1;
	ProcessPCBs[1]->RTCsetting = RTC_DEFAULT_RATE;
	
	file_desc_table = ProcessPCBs[1]->process_file_desc_table; //set this file descriptor table as the active one (file_desc_table declared in file_descriptor.h)
		
	int32_t fd = open("shell"); //open the shell file for validy checking (look at header)
	read(fd, executable_header, 40); //read in the header
	//check magic number, just for the heck of it
	if((executable_header[0] != MAGIC0) || (executable_header[1] != MAGIC1) || (executable_header[2] != MAGIC2) || (executable_header[3] != MAGIC3))
	{
		clear();
		printf("\nMagic number mismatch while starting inital shells\n");
		while(1); //BSOD hang here, since this is a really really weird fault...should never ever happen
	}
	
	//read in offset
	offset = (executable_header[27] << 24) + (executable_header[26] << 16) + (executable_header[25] << 8) + (executable_header[24] << 0);
	close(fd); //reset to begining of the shell program by simply reopening the file

	//fill in PCB for new shell in terminal 2. We can do this now because there is no reason the shell should not launch/execute
	ProcessPCBs[3]->parent_process = 0; //the shell was started at boot - it has no parent
	ProcessPCBs[3]->terminal_num = 2; //inital shell runs on terminal 0
	for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table
	{
		ProcessPCBs[3]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
		ProcessPCBs[3]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
	}
	ProcessPCBs[3]->process_file_desc_table[0].flags |= DESC_IN_USE; //Fill in FD table properly - open stdin/stdio
	ProcessPCBs[3]->process_file_desc_table[0].file_ops = &file_ops[FILE_TYPE_STDIN];
	ProcessPCBs[3]->process_file_desc_table[1].flags |= DESC_IN_USE;
	ProcessPCBs[3]->process_file_desc_table[1].file_ops = &file_ops[FILE_TYPE_STDOUT];
	ProcessPCBs[3]->eip = offset;
	ProcessPCBs[3]->esp = UserStackAddr[3];
	ProcessPCBs[3]->ebp = UserStackAddr[3];
	ProcessPCBs[3]->kernel_esp = KernelStackAddr[3];
	ProcessPCBs[3]->lengthofargs = 0;
	ProcessPCBs[3]->ds = USER_DS;
	ProcessPCBs[3]->cs = USER_CS;
	ProcessPCBs[3]->ss = USER_DS;
	ProcessPCBs[3]->RTCsetting = RTC_DEFAULT_RATE;
	
	
	//fill in PCB for new shell in terminal 1. We can do this now because there is no reason the shell should not launch/execute	
	ProcessPCBs[2]->parent_process = 0; //the shell was started at boot - it has no parent
	ProcessPCBs[2]->terminal_num = 1; //inital shell runs on terminal 0
	for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table
	{
		ProcessPCBs[2]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
		ProcessPCBs[2]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
	}
	ProcessPCBs[2]->process_file_desc_table[0].flags |= DESC_IN_USE; //Fill in FD table properly - open stdin/stdio
	ProcessPCBs[2]->process_file_desc_table[0].file_ops = &file_ops[FILE_TYPE_STDIN];
	ProcessPCBs[2]->process_file_desc_table[1].flags |= DESC_IN_USE;
	ProcessPCBs[2]->process_file_desc_table[1].file_ops = &file_ops[FILE_TYPE_STDOUT];
	ProcessPCBs[2]->eip = offset;
	ProcessPCBs[2]->esp = UserStackAddr[2];
	ProcessPCBs[2]->ebp = UserStackAddr[2];
	ProcessPCBs[2]->kernel_esp = KernelStackAddr[2];
	ProcessPCBs[2]->lengthofargs = 0;
	ProcessPCBs[2]->ds = USER_DS;
	ProcessPCBs[2]->cs = USER_CS;
	ProcessPCBs[2]->ss = USER_DS;
	ProcessPCBs[3]->RTCsetting = RTC_DEFAULT_RATE;

	
	//copy shell program for all three inital shells
	fd = open("shell");
	map_process_page(PROCESS_3_PHYS_ADDR); //map page for first process (shell)
	filesize = read(fd, (void *)(PROCESS_3_VIRT_ADDR+OFFSET), FULLPAGE); //read the entire shell program (minus the header) into the process1 page with offset
	close(fd); //close out the file descriptor, now unneeded.
	
	fd = open("shell");
	map_process_page(PROCESS_2_PHYS_ADDR); //map page for first process (shell)
	filesize = read(fd, (void *)(PROCESS_2_VIRT_ADDR+OFFSET), FULLPAGE); //read the entire shell program (minus the header) into the process1 page with offset
	close(fd); //close out the file descriptor, now unneeded.

	fd = open("shell");
	map_process_page(PROCESS_1_PHYS_ADDR); //map page for first process (shell)
	filesize = read(fd, (void *)(PROCESS_1_VIRT_ADDR+OFFSET), FULLPAGE); //read the entire shell program (minus the header) into the process1 page with offset
	close(fd); //close out the file descriptor, now unneeded.
	
	//set tss for new kernel stack
	tss.ss0 = KERNEL_DS; //segment where the kernel stack is located
	tss.esp0 = PROCESS_1_KERNEL_STACK; //kernel stack base
	
	current_process = 1; //first process will be in termial 0
	current_terminal = 0;
	
	//Set up terminal stuff
	terms[0].process_on_this_term = 1;
	terms[1].process_on_this_term = 2;
	terms[2].process_on_this_term = 3;
	
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '0';
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM0ATTRIB;
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
	
	//execute assembly to switch to user mode - this chunk of assembly should never return
	asm volatile(
		
		//set up stack for iret
		"pushl %1   \n\t" 			//push DS entry
		"pushl %3   \n\t" 			//push new esp argument
		"pushf   	\n\t" 			//push current eflags to new process
		"pushl %0   \n\t" 			//push CS entry
		"pushl %2   \n\t" 			//push entry point
		
		//do some miscelanous non-stack things
		"movw %4, %%ax   \n\t"
		"movw %%ax, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
		"movl %3, %%ebp   \n\t" //set up new stack base pointer
		
		"sti   \n\t"
		//jumpt to the user space program
		"iret   \n\t"	   			//execute the new program
		
		:
		:"g" (csarg), "g" (dsarg), "g" (offset), "g" (PROCESS_1_STACK_BASE), "g" (USER_DS) 
		:"memory", "eax", "ebp"
		);
	
	
	//we should never hit this point, so return minus one cuz something wiggy happened!
	clear();
	printf("\nyou are returning from the inital shell launch function!!!\n This is weird!!!\n look at process.c, start_initial_shells()\n\n");
	return -1;
	
}

/*
 * switch_process
 *   DESCRIPTION: stores program state of one process, and sets another process to run in userspace. Used for switching between processes either
 *			      on a terminal switch, or on an RTC-triggered scheduler process switch
 *   INPUTS: PIDout - the process to take out of memory
 *			 PIDin - the process to resume execution of
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 for something invalid (process not currently the most-recently created process in that terminal, etc..)
 *   SIDE EFFECTS: changes what is currently executing
 */
int32_t switch_process(int PIDout, int PIDin) {
	cli();
	send_eoi_all_irq();

	//check for validity of in and out processes - BSOD if they're not valid
	if(PIDinUse[PIDout] != 1 || PIDinUse[PIDin] != 1)
	{
		printk("\n!!~~~~~~~~~~~~!!\nERROR: attempted to switch between non-existant processes (%d and %d)!!! \n!!~~~~~~~~~~~~!!\n\n", PIDin, PIDout);
		while(1);
	}
	
	else if(PIDout == PIDin)
	{
		printk("\n!!~~~~~~~~~~~~!!\nERROR: attempted to switch between the same process (%d)!!! \n!!~~~~~~~~~~~~!!\n\n", PIDin);
		while(1);
	}
	
	else if(PIDin == current_process)
	{
		printk("\n!!~~~~~~~~~~~~!!\nERROR: attempted to switch to the current process (%d)!!! \n!!~~~~~~~~~~~~!!\n\n", PIDin);
		while(1);
	}
	
	else if(PIDout != current_process)
	{
		printk("\n!!~~~~~~~~~~~~!!\nERROR: attempted to switch from a process other than the current process(%d)!!! \n!!~~~~~~~~~~~~!!\n\n", PIDout);
		while(1);
	}
	
	file_desc_table = ProcessPCBs[PIDin]->process_file_desc_table; //set the new process's file descriptor table as the active one (file_desc_table declared in file_descriptor.h)
	
	//close the RTC for this process
	//rtc_close();
		
	tss.ss0 = KERNEL_DS;
	tss.esp0 = KernelStackAddr[PIDin]; //kernel stack base

	// if (ProcessPCBs[PIDout]->cs == KERNEL_CS)
		// printk("switching from kernel-space (%d)", PIDout);
	// else
		// printk("switching from user-space (%d)", PIDout);
	
	// if (ProcessPCBs[PIDin]->cs == KERNEL_CS)
		// printk(" to kernel-space (%d)\n", PIDin);
	// else
		// printk(" to user-space (%d)\n", PIDin);
	
	//printPCB(PIDin); 
	//printPCB(PIDout);
	
	current_process = PIDin; //mark new process as active
	map_user_vga_page((uint32_t) terms[ProcessPCBs[current_process]->terminal_num].video_mem_start);
	map_process_page(ProcessPhysAddr[PIDin]); //restore process page. Flushes the TLB.

	
	if(ProcessPCBs[current_process]->cs == KERNEL_CS)
	{
		asm volatile(
		
			//set up stack for iret
			"movl %3, %%esp   \n\t" 			//set new esp argument
			
			"xor %%edi, %%edi	\n\t"		
			"orl %6, %%edi  	\n\t" 	//push current eflags to new process
			"pushl %%edi		\n\t"
			
			"pushl %0   \n\t" 			//push CS entry
			"pushl %2   \n\t" 			//push entry point
			
			//do some miscelanous non-stack things
			"movw %4, %%ax   \n\t"
			"movw %%ax, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!

			
			//restore registers from previous process
			"movl	%5, %%edi \n\t" 				// set destination register to the memory location of the old process PCB	
			"movl	0(%%edi), %%eax \n\t"			// restore EAX		
			"movl	4(%%edi), %%ebx \n\t"			// restore EBX
			"movl	8(%%edi), %%ecx  \n\t"			// restore ECX
			"movl	12(%%edi), %%edx  \n\t"			// restore EDX
			"movl	16(%%edi), %%ebp  \n\t"			// restore EBP
													// ESP and EIP during iret
			"movl	28(%%edi), %%esi  \n\t"			// restore ESI										
			"movl	32(%%edi), %%edi \n\t"			// move the old EDI value to EDI. As this overwrites our working pointer, it should be done last.
			"sti 					\n\t"
			//jumpt to the new program
			
			"iret   \n\t"	   			//execute the new program
			
			:
			:	"g" ((uint32_t)ProcessPCBs[PIDin]->cs), //0
				"g" ((uint32_t)ProcessPCBs[PIDin]->ss), //1
				"g" (ProcessPCBs[PIDin]->eip), 			//2
				"g" (ProcessPCBs[PIDin]->kernel_esp), 	//3
				"g" ((uint32_t)ProcessPCBs[PIDin]->ds), //4
				"g" (ProcessPCBs[PIDin]), 				//5
				"g" ((ProcessPCBs[PIDin])->eflags)		//6	
			//:"g" ((uint32_t)ProcessPCBs[PIDin]->cs), "g" ((uint32_t)ProcessPCBs[PIDin]->ss), "g" (ProcessPCBs[PIDin]->eip), "g" (ProcessPCBs[PIDin]->kernel_esp), "g" ((uint32_t)ProcessPCBs[PIDin]->ds), "g" (ProcessPCBs[PIDin]), "g" ((ProcessPCBs[PIDin])->eflags)
			:"memory", "eax", "ebx", "ecx", "edx", "edi"
		);

	}
	else
	{

		handle_current_pending_signals();

		asm volatile(
		
			//set up stack for iret
			"pushl %1   \n\t" 			//push SS entry
			"pushl %3   \n\t" 			//push new esp argument
			
			"xor %%edi, %%edi	\n\t"		
			"orl %6, %%edi  	\n\t" 	//push current eflags to new process
			"pushl %%edi		\n\t"
			
			"pushl %0   \n\t" 			//push CS entry
			"pushl %2   \n\t" 			//push entry point
			
			//do some miscelanous non-stack things
			"movw %4, %%ax   \n\t"
			"movw %%ax, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!

			
			//restore registers from previous process
			"movl	%5, %%edi \n\t" 				// set destination register to the memory location of the old process PCB	
			"movl	0(%%edi), %%eax \n\t"			// restore EAX		
			"movl	4(%%edi), %%ebx \n\t"			// restore EBX
			"movl	8(%%edi), %%ecx  \n\t"			// restore ECX
			"movl	12(%%edi), %%edx  \n\t"			// restore EDX
			"movl	16(%%edi), %%ebp  \n\t"			// restore EBP
													// ESP and EIP during iret
			"movl	28(%%edi), %%esi  \n\t"			// restore ESI										
			"movl	32(%%edi), %%edi \n\t"			// move the old EDI value to EDI. As this overwrites our working pointer, it should be done last.
			"sti 					\n\t"
			//jumpt to the new program
			"iret   \n\t"	   			//execute the new program
			
			:
			:"g" ((uint32_t)ProcessPCBs[PIDin]->cs), "g" ((uint32_t)ProcessPCBs[PIDin]->ss), "g" (ProcessPCBs[PIDin]->eip), "g" (ProcessPCBs[PIDin]->esp), "g" ((uint32_t)ProcessPCBs[PIDin]->ds), "g" (ProcessPCBs[PIDin]), "g" ((ProcessPCBs[PIDin])->eflags)			
			:"memory", "eax", "ebx", "ecx", "edx", "edi"
		);
	}
		//iret, so should never hit this point in the code
		return 3;	
}

/*
 * printPCB
 *   DESCRIPTION: debugging function, prints out pcb to screen
 *   INPUTS: pid - which PID's pcb to print out
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void printPCB (uint32_t pid){
	
	printk("************** \n");
	printk("PCB for process %d:\n", pid);
	printk("eax = 0x%#x | \n", ProcessPCBs[pid]->eax);
	printk("ebx = 0x%#x | \n", ProcessPCBs[pid]->ebx); 
	printk("ecx = 0x%#x | \n", ProcessPCBs[pid]->ecx); 
	printk("edx = 0x%#x | \n", ProcessPCBs[pid]->edx); 
	printk("ebp = 0x%#x | \n", ProcessPCBs[pid]->ebp);
	printk("esp = 0x%#x | \n", ProcessPCBs[pid]->esp);
	printk("eip = 0x%#x | \n", ProcessPCBs[pid]->eip);	
	printk("esi = 0x%#x | \n", ProcessPCBs[pid]->esi);
	printk("edi = 0x%#x | \n", ProcessPCBs[pid]->edi);
	printk("kernel_esp = 0x%#x | \n", ProcessPCBs[pid]->kernel_esp);
	printk("ds = 0x%#x | \n", ProcessPCBs[pid]->ds);
	printk("cs = 0x%#x | \n", ProcessPCBs[pid]->cs);
	printk("ss = 0x%#x | \n", ProcessPCBs[pid]->ss);
	printk("eflags = 0x%#x | \n", ProcessPCBs[pid]->eflags);
	printk("terminal_num = %d | \n", ProcessPCBs[pid]->terminal_num); 
	printk("parent_process = %d | \n", ProcessPCBs[pid]->parent_process); 
	if (pid == current_process)
		printk("Not the current process"); 
	else
		printk("This is the current process"); 		
	printk("\n**************\n");		


}

/*
 * halt_proc
 *   DESCRIPTION: Terminates a process.
 *   INPUTS: status -- the status to be returned to the caller.
 *   OUTPUTS: none.
 *   RETURN VALUE: none.
 *   SIDE EFFECTS: returns control back to the caller process.
 */
void halt_proc(uint16_t proc_id, uint16_t status) {
	cli();
	int i = 0; //counter variable

	if(proc_id == 1 || proc_id == 2 || proc_id == 3)
	{
		//a halt call from the inital shell should be interpreted as "shutdown the machine"
		//do any cleanup required, and then spin nicely until the user actually exits the VM
		printf("\nERROR: Tried to KILL the original shell!\n");
		while(1);
	}
	else
	{
		for(i = 0; i < MAX_OPEN_FILES; i++)	//clear out FD table for the halting function (for security purposes)
		{
			ProcessPCBs[proc_id]->process_file_desc_table[i].flags &= ~DESC_IN_USE; //set all as unused
			ProcessPCBs[proc_id]->process_file_desc_table[i].file_ops = NULL; //set all as undescribed
		}
		
		file_desc_table = ProcessPCBs[ProcessPCBs[proc_id]->parent_process]->process_file_desc_table; //set the old file descriptor table as the active one (file_desc_table declared in file_descriptor.h)
		
		send_eoi_all_irq();

		//close the RTC for this process
		rtc_close();
		
		clear_signals(proc_id);

		//set tss for old kernel stack
		tss.ss0 = KERNEL_DS; //segment where the kernel stack is located
		//tss.esp0 = ProcessPCBs[ProcessPCBs[proc_id]->parent_process]->kernel_esp; //old esp from last kernel stack, from before execute happened
		tss.esp0 = KernelStackAddr[ProcessPCBs[proc_id]->parent_process]; //kernel stack base
		
		PIDinUse[proc_id] = 0; //clear entry, as this process is no longer taking up memory space
		current_process = ProcessPCBs[proc_id]->parent_process; //set global variable to show the old process is now running
																		//this will eventually be set only by the scheduler.		
		terms[ProcessPCBs[proc_id]->terminal_num].process_on_this_term = ProcessPCBs[proc_id]->parent_process;

		map_process_page(ProcessPhysAddr[ProcessPCBs[proc_id]->parent_process]); 

		asm volatile(
		"movl 	%6, %%eax		\n\t" 			//set return value into EAX
		
		//set up stack for iret
		"pushl %1   \n\t" 			//push DS entry
		"pushl %3   \n\t" 			//push previous esp argument
		
		"xor %%edi, %%edi		\n\t"		
		"orl %8, %%edi  		\n\t" 			//push current eflags to new process
		"pushl %%edi			\n\t"
		// "pushf \n"
		
		"pushl %0   \n\t" 			//push CS entry
		"pushl %2   \n\t" 			//push code return point


		
		//restore registers from previous process
		"movl	%7, %%edi \n\t" 				// set destination register to the memory location of the old process PCB
												// EAX can be used as a working register since it will be overwritten at the end anyway
		"movl	4(%%edi), %%ebx \n\t"			// restore EBX
		"movl	8(%%edi), %%ecx  \n\t"			// restore ECX
		"movl	12(%%edi), %%edx  \n\t"			// restore EDX
		"movl	16(%%edi), %%ebp  \n\t"			// restore EBP
												// ESP and EIP restored below
		"movl	28(%%edi), %%esi  \n\t"			// restore ESI								
		"movl	32(%%edi), %%edi \n\t"			// move the old EDI value to EDI. As this overwrites our working pointer, it should be done last.

		//do some miscelanous non-stack things
		"movw %4, %%di   \n\t"  //for some reason, can't store directly to %ds from memory, use edi as temporary
		"movw %%di, %%ds  \n\t" //change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
		//"movl %5, %%ebp   \n\t" //set up old stack base pointer
		//AJ: We already restored ebp. Restoring it againg causes a pagefault	
		
		"sti 	\n\t"
		//jump to the user space program
		"iret   \n\t"	   			//jump back to the old program

		:
		: 	"g" (USER_CS), 
			"g" (USER_DS), 
			"g" (ProcessPCBs[current_process]->eip), 
			"g" (ProcessPCBs[current_process]->esp), 
			"g" (USER_DS), 
			"g" (ProcessPCBs[current_process]->ebp), 
			"g" (status), 
			"g" (ProcessPCBs[current_process]), 
			"g" (ProcessPCBs[current_process]->eflags)
		:"memory", "eax", "ebx", "ecx", "edx", "edi", "ebp"
		);

	}

	//we should never hit this point, so return minus one cuz something wiggy happened!
	clear();
	printf("\nyou are returning from the halt system call!!!\n This is weird!!!\n look at process.c, halt_proc()\n\n");

}

