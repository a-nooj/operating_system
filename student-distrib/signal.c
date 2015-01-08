/* signal.c - signal related functions */

#include "signal.h"

extern PCB * ProcessPCBs[MAX_PROCESSES+1]; //reference the process pcbs from elsewhere (process.c)

extern volatile int current_process; //integer representing the currently executing process

extern uint32_t ProcessPhysAddr[MAX_PROCESSES+1];

//Local functions
// static void kill_task(int proc_id);
// static void ignore_signal(void);
void * default_handler(uint32_t signum);
void handle_signal(int signum, int proc_id);

/*
 * set_handler
 *   DESCRIPTION: Sets the handler for a signal.
 *   INPUTS: signum -- the signal number.
 *           handler_address -- the handler to associate with the signal.
 *   OUTPUTS: none.
 *   RETURN VALUE: 0 on success, -1 on failure.
 *   SIDE EFFECTS: sets a signal handler.
 */
int32_t set_handler(int32_t signum, void* handler_address) {
	int caller_process = current_process;
	
	//check signum amd handler address
	if (signum < 0 || signum > NUM_SIGS)
		return -1;
	else if (handler_address == NULL) {
		ProcessPCBs [caller_process]->handler[signum] = default_handler(signum);
		return -1;
	}
	
	ProcessPCBs[caller_process]->handler[signum] = handler_address; 
	
	return 0;
}


/*
 * sigreturn
 *   DESCRIPTION: Returns from a signal.
 *   INPUTS: none.
 *   OUTPUTS: none.
 *   RETURN VALUE: Returns the hardware context value of EAX to avoid special casing
 *   SIDE EFFECTS: copies hardware context.
 */
int32_t sigreturn(void) {
	map_process_page(ProcessPhysAddr[current_process]); 

	//BREAK
	printPCB(current_process);

	asm volatile( 
	"xorl %%ebx, %%ebx						\n"
	"movw %[user_ds], %%bx   				\n"  	//for some reason, can't store directly to %ds from memory, use edi as temporary
	"movw %%bx, %%ds  						\n" 	//change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
	"movl %[user_esp], %%ebx				\n" 
	
	//Fix register values
	"movl 4(%%ebx), %%eax					\n"
	"movl %%eax, %[ebx]						\n"

	"movl 8(%%ebx), %%eax					\n"
	"movl %%eax, %[ecx]						\n"

	"movl 16(%%ebx), %%eax					\n"
	"movl %%eax, %[edx]						\n"

	"movl 20(%%ebx), %%eax					\n"
	"movl %%eax, %[esi]						\n"

	"movl 24(%%ebx), %%eax					\n"
	"movl %%eax, %[edi]						\n"

	"movl 28(%%ebx), %%eax					\n"
	"movl %%eax, %[ebp]						\n"

	"movl 32(%%ebx), %%eax					\n"
	"movl %%eax, %[eax]						\n"

	"movl 36(%%ebx), %%eax					\n"
	"movw %%ax, %[ds]						\n"

	"movl 56(%%ebx), %%eax					\n"
	"movl %%eax, %[eip]						\n"

	"movl 60(%%ebx), %%eax					\n"
	"movw %%ax, %[cs]						\n"

	"movl 64(%%ebx), %%eax					\n"
	"movl %%eax, %[eflags]					\n"

	"movl 68(%%ebx), %%eax					\n"
	"movl %%eax, %[user_esp]				\n"

	"movl 72(%%ebx), %%eax					\n"
	"movw %%ax, %[ss]						\n"
	
	// // Fix ESP value
	// "addl $92, %%ebx						\n"
	// "movl %[user_esp], %%ebx				\n" 
	
	// Execute (in user space) the handler
	"pushl %[user_ds]						\n"
	"pushl %[user_esp]						\n"
	"pushl %[eflags]						\n"
	"pushl %[user_cs]						\n"
	"pushl %[eip]							\n"
	"movl %[eax], %%eax						\n"
	"movl %[ebx], %%ebx						\n"
	"movl %[ecx], %%ecx						\n"
	"movl %[edx], %%edx						\n"
	"movl %[esi], %%esi						\n"
	"movl %[edi], %%eax						\n"
	"movl %[ebp], %%ebp						\n" //DO THIS LAST!!!
	"iret									\n" 
	:	//outputs
	
	: 	//inputs
		[user_esp] 	"g" (ProcessPCBs[current_process]->esp),
		[user_ds] 	"g" (USER_DS),	
		[user_cs] 	"g" (USER_CS),
		
		[eflags] 	"g" (ProcessPCBs[current_process]->eflags),
		[eax] 	"g"	(ProcessPCBs[current_process]->eax),
		[ebx] 	"g"	(ProcessPCBs[current_process]->ebx),
		[ecx] 	"g"	(ProcessPCBs[current_process]->ecx),
		[edx] 	"g"	(ProcessPCBs[current_process]->edx),
		[esi] 	"g"	(ProcessPCBs[current_process]->esi),
		[edi] 	"g"	(ProcessPCBs[current_process]->edi),
		[ebp] 	"g"	(ProcessPCBs[current_process]->ebp),
		[eip] 	"g"	(ProcessPCBs[current_process]->eip),
		
		[ss] 	"g"	(ProcessPCBs[current_process]->ss),
		[cs] 	"g"	(ProcessPCBs[current_process]->cs),
		[ds] 	"g"	(ProcessPCBs[current_process]->ds)
			
	: 	//clobber list
		"%ebx",
		"%eax"
	);	
	return 0;
}

void signals_init(){
	int i, j;
	for (i = 0; i <= MAX_PROCESSES; i++) {
		for (j=0; j < NUM_SIGS; j++) {
			ProcessPCBs[i]->pending[j] = 0; //No signals pending
			ProcessPCBs[i]->masked[j] = 0; //All signals UN-masked
			ProcessPCBs[i]->handler[j] = default_handler(j); //All sighandlers default
		}
	}
}
		
void * default_handler(uint32_t signum) {
	if ( signum == 0 || signum == 1 || signum == 2 )
		return (void *) KILL_TASK;
	else if ( signum == 3 || signum == 4 )
		return (void *) IGNORE_SIG;
	else {
		printf("ERROR: Invalid signum %d", signum);
		return (void *) -1;
	}
}

void send_signal(int signum, int proc_id) {

	//Check to see if this signal is masked
	if ( ProcessPCBs[proc_id]->masked[signum] )
		return;
	
	printk("sending SIG %d to proc %d\n", signum, proc_id);
	
	//This signal is now pending for this process
	ProcessPCBs[proc_id]->pending[signum] = 1; 
}

//Checks for pending signals for the specified process
//and handles them
void handle_current_pending_signals () {
	int i;
	
	if(ProcessPCBs[current_process]->ds == KERNEL_DS)
		return;
	
	for (i=0; i < NUM_SIGS; i++)
		if ( ProcessPCBs[current_process]->pending[i] )
			handle_signal (i, current_process);
}

//Checks for pending signals for the specified process
//and handles them
void handle_pending_signals (int proc_id) { 
	int i;
	
	if(ProcessPCBs[proc_id]->ds == KERNEL_DS)
		return;
	
	for (i=0; i < NUM_SIGS; i++)
		if ( ProcessPCBs[proc_id]->pending[i] )
			handle_signal (i, proc_id);
}

void clear_signals(int proc_id) {
	int i;
	for (i=0; i < NUM_SIGS; i++) {
		ProcessPCBs[proc_id]->pending[i] = 0; //No signals pending
		ProcessPCBs[proc_id]->masked[i] = 0; //All signals UN-masked
		ProcessPCBs[proc_id]->handler[i] = default_handler(i); //All sighandlers default
	}
}
	
void handle_signal(int signum, int proc_id) {
	int i;
	uint32_t user_sigreturn_linkage; 
	
	cli(); //Just in case
	//Mask all other signals
	for (i=0; i < NUM_SIGS; i++) {
		if (i != signum )
			ProcessPCBs[proc_id]->masked[i] = 1;
	}

	//Signal is no longer pending
	ProcessPCBs[proc_id]->pending[signum] = 0; 

	if (ProcessPCBs[proc_id]->handler[signum] == (void*) KILL_TASK && (signum == SEGFAULT || signum == DIV_ZERO))
		halt_proc(proc_id, 256);
	if (ProcessPCBs[proc_id]->handler[signum] == (void*) KILL_TASK && signum == INTERRUPT)
		halt_proc(proc_id, 1);		
	else if (ProcessPCBs[proc_id]->handler[signum] == (void*) IGNORE_SIG)
		return;
	else if (ProcessPCBs[proc_id]->handler[signum] != NULL){
	
		printPCB(proc_id);
	
		map_process_page(ProcessPhysAddr[proc_id]); 

		//BREAK

		// Set up signal handler's stack frame
		// - push asm code for exectuing sigreturn
		// - push processes hardware context 
		// - push signum to user stack
		// - push retaddr to user stack
		// Execute (in user space) the handler 
		// Call sigreturn (user program will do this automatically if retaddr is setup correctly
		asm volatile( 
		"xorl %%ebx, %%ebx						\n"
		"movw %[user_ds], %%bx   				\n"  	//for some reason, can't store directly to %ds from memory, use edi as temporary
		"movw %%bx, %%ds  						\n" 	//change Data Segment in ds register before jump  !!!!USER_DS = 0x002B!!!!
		"movl %[user_esp], %%ebx				\n" 
		
		// //Set up return linkage
		"movl $0x80cd00, %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"
		
		"movl $0xab8, %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"
		"movl %%ebx, %[user_sigreturn_linkage]	\n" // save sigreturn linkage addr
		
		// - push processor context 
		"movl %[user_ds], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"	

		"movl %[user_esp], %%eax				\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"

		"movl %[eflags], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		

		"movl %[user_cs], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		

		"movl %[eip], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
		
		// - push dummy handler info
		"subl $4, %%ebx							\n"
		"movl $0, (%%ebx)						\n"		
		
		"subl $4, %%ebx							\n"
		"movl $0, (%%ebx)						\n"			
	
		// - push hw segment context
		"movl %[user_ds], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"			

		"movl %[user_ds], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"				

		"movl %[user_ds], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		

		// -push user registers
		"movl %[eax], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
		
		"movl %[ebp], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		"movl %[edi], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		"movl %[esi], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		"movl %[edx], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		"movl %[ecx], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		"movl %[ebx], %%eax						\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		// - push signum to user stack
		"movl %[signum], %%eax					\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"		
				
		// - push retaddr to user stack
		"movl %[user_sigreturn_linkage], %%eax	\n"
		"subl $4, %%ebx							\n"
		"movl %%eax, (%%ebx)					\n"
		
		//Fix the user stack pointer
		"movl %%ebx, %[user_esp]				\n"

		// Execute (in user space) the handler
		"pushl %[user_ds]						\n"
		"pushl %[user_esp]						\n"
		"pushl %[eflags]						\n"
		"pushl %[user_cs]						\n"
		"pushl %[sighandler]					\n"
		"movl %[eax], %%eax						\n"
		"movl %[ebx], %%ebx						\n"
		"movl %[ecx], %%ecx						\n"
		"movl %[edx], %%edx						\n"
		"movl %[esi], %%esi						\n"
		"movl %[edi], %%eax						\n"
		"movl %[ebp], %%ebp						\n" //DO THIS LAST!!!
		"iret									\n" 
		
		//Data for the sigreturn linkage
		"sigreturn_asm_linkage: 				\n"
		"movl $10, %%eax						\n"
		"int $0x80								\n"
		
		:	//outputs
		
		: 	//inputs
			[user_esp] 	"g" (ProcessPCBs[proc_id]->esp),
			[user_ds] 	"g" (USER_DS),	
			[user_cs] 	"g" (USER_CS),
			
			[eflags] 	"g"	(ProcessPCBs[proc_id]->eflags),
			[eax] 	"g"	(ProcessPCBs[proc_id]->eax),
			[ebx] 	"g"	(ProcessPCBs[proc_id]->ebx),
			[ecx] 	"g"	(ProcessPCBs[proc_id]->ecx),
			[edx] 	"g"	(ProcessPCBs[proc_id]->edx),
			[esi] 	"g"	(ProcessPCBs[proc_id]->esi),
			[edi] 	"g"	(ProcessPCBs[proc_id]->edi),
			[ebp] 	"g"	(ProcessPCBs[proc_id]->ebp),
			[eip] 	"g"	(ProcessPCBs[proc_id]->eip),
			
			[sighandler] 	"g"	(ProcessPCBs[proc_id]->handler[signum]),
			[signum] 	"g"	(signum),
			[user_sigreturn_linkage] "g" (user_sigreturn_linkage)
		: 	//clobber list
			"%eax", 
			"%ebx"
		);	
	}
}
