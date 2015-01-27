#include "syscall.h"

static int32_t pid = -1;
pcb_t* process0_pcb = PROCESS0_PCB;
pcb_t* process1_pcb = PROCESS1_PCB;
pcb_t* current_pcb;

/* Operations Table */
ops_t file_ops = {.open=file_open, .close=file_close, .read=file_read, .write=file_write};
ops_t dir_ops = {.open=dir_open, .close=dir_close, .read=dir_read, .write=dir_write};
ops_t rtc_ops = {.open=rtc_open, .close=rtc_close, .read=rtc_read, .write=rtc_write};
ops_t stdin_ops = {.open=term_open, .close=term_close, .read=term_read, .write=NULL};
ops_t stdout_ops = {.open=NULL, .close=NULL, .read=NULL, .write=term_write};

int32_t syscall_halt(uint8_t status) {
	/* If halting initial shell process */
	if (current_pcb->pid == 0) {
		printf("Shutting down OS\n");
		while(1) {
			asm("hlt");
		}
	}
	
	/* Change TSS to use parent's kernel stack on syscalls */
	tss.esp0 = PROCESS0_KERNEL_STACK;
	//current_pcb->file_array[fd].ops->close(fd);

	/* Load parent's page directory */
	asm volatile("movl %0, %%cr3"
	: /* no outputs */
	: "r" (process0_dir) /* inputs */
	);
	
	current_pcb = process0_pcb;
	
	return 0;
}

int32_t syscall_execute(const uint8_t* command) {
	uint8_t cmd[NAME_LEN];
	int32_t* p_dir;
	pid++;	
	/* Check for max number of processes */
	if (pid >= 2)
		return -1;
	
	/* Set current PCB depending on pid */
	switch (pid) {
		case 0:
			current_pcb = PROCESS0_PCB;
			current_pcb->pid = pid;
			current_pcb->p_dir = (uint32_t*)process0_dir;
			current_pcb->parent_process = NULL;
			break;
		case 1:
			current_pcb = PROCESS1_PCB;
			current_pcb->pid = pid;
			current_pcb->p_dir = (uint32_t*)process1_dir;
			current_pcb->parent_process = PROCESS0_PCB;
			break;
		default:
			return -1;
	}
	
	/* Initialize stdin and stdout */
	init_stds(current_pcb);

	/* Parse command line and separate arguments */
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t entry_point;
	
	while (command[i] != '\0') {
		if (command[i] == ' ') {
			while (command[i] == ' ') {
				i++;
			}
			while (command[i] != '\0') {
				current_pcb->arg[j] = command[i];
				i++;
				j++;
			}
			current_pcb->arg[j] = '\0';
			break;
		}
		cmd[i] = command[i];
		i++;
	}
	
	/* Open file and check for valid magic */
	uint8_t prog[4];
	dentry_t temp;
	
	/* Get directory entry associated with command name */
	if (read_dentry_by_name((uint8_t*)cmd, &temp) == -1)
		return -1;
	
	/* Read data from inode number of directory entry */
	read_data(temp.inode_num, 0, prog, 4);//read_file_length(temp.inode_num));
	
	/* Enable page directory for process 0 */
	//process0_dir[PROG_VIRT_ADDR >> PD_SHIFT] |= P_FLAG;

	/* Checks for magic number */
	if ((prog[0] != INITIAL_BYTE) || (prog[1] != E) || (prog[2] != L) || (prog[3] != F))
		return -1;
	
	/* Enable respective page directories */
	switch (pid) {
		case 0:
			process0_dir[PROCESS0_KERNEL_STACK >> PD_SHIFT] |= P_FLAG;
			p_dir = process0_dir;
			break;
		case 1:
			process1_dir[PROCESS1_KERNEL_STACK >> PD_SHIFT] |= P_FLAG;
			p_dir = process1_dir;
			break;
		default:
			break;
	}	
	/* Load page directory into CR3 */
	asm volatile("movl %0, %%cr3"
	: /* no outputs */
	: "r" (p_dir) /* inputs */
	);
	
	//current_pcb->file_array[fd].ops->open(fd);!!!!!!!!!!!
	//term_open(0);
	//sti();
	/* Read executable image into virtual memory 128MB */
	read_data(temp.inode_num, 0, (uint8_t*)(PROG_IMG_ADDR), read_file_length(temp.inode_num));
	
	/* Double check for magic and file copied correctly */
	if ((*(uint8_t*)(PROG_IMG_ADDR) != INITIAL_BYTE) || (*(uint8_t*)(PROG_IMG_ADDR + 1) != E) || (*(uint8_t*)(PROG_IMG_ADDR + 2) != L)
		|| (*(uint8_t*)(PROG_IMG_ADDR + 3) != F))
		return -1;
	
	/* Get entry point in bytes 24 - 27 of executable */
	read_data(temp.inode_num, 24, (uint8_t*)&entry_point, 4);
	
	/* Change TSS */
	tss.ss0 = KERNEL_DS;
	switch (pid) {
		case PROC0:
			tss.esp0 = PROCESS0_KERNEL_STACK;
			break;
		case PROC1:
			tss.esp0 = PROCESS1_KERNEL_STACK;
			break;
		default:
			break;
	}
	
	/* Save current process stack pointer */
	current_pcb->esp = (uint32_t)&command;
	
	/* IRET to switch to privilege level 3 */
	asm volatile(
		"pushl %0\n\t"
		"pushl %1\n\t"
		"pushl %4\n\t"
		"pushl %2\n\t"
		"pushl %3\n\t"
		"movl %0, %%ds\n\t"
		"iret\n\t"
		:
		: "r"(USER_DS), "r"(USER_STACK), "r"(USER_CS), "r"(entry_point), "r"(FLAGS)
		: "memory", "cc"
		);
		
	return 0;
}

/* 
* int32_t syscall_read(int32_t fd, void* buf, int32_t nbytes)
*	Inputs: int32_t fd = file descriptor
*			void * buf = pointer to buffer
*			int32_t nbytes = number of bytes to read
*	Return Value: -1 on failure, 0 on EOF for files or read
*					RTC, N bytes read for read files
*	Function: Read system call that reads data from file,
*				RTC, directory, and keyboard
*/
int32_t syscall_read(int32_t fd, void* buf, int32_t nbytes) {
	/* Check for valid file descriptor */
	if (fd < STDIN_FILE || fd > MAX_FILES-1)
		return -1;
	
	/* Check if stdout */
	if (fd == STDOUT_FILE)
		return -1;
	
	/* Check if file descriptor has been initialized */
	if (current_pcb->file_array[fd].flags == FREE_)
		return -1;

	return current_pcb->file_array[fd].ops->read(fd, buf, nbytes);
}

/* 
* int32_t syscall_write(int32_t fd, const void* buf, int32_t nbytes)
*	Inputs: int32_t fd = file descriptor
*			void * buf = pointer to buffer
*			int32_t nbytes = number of bytes to read
*	Return Value: -1 on failure, 0 on EOF for files or read
*					RTC, N bytes read for read files
*	Function: Write system call that writes data to terminal or RTC
*/
int32_t syscall_write(int32_t fd, const void* buf, int32_t nbytes) {
	/* Check for valid file descriptor */
	if (fd < STDIN_FILE || fd > MAX_FILES-1)
		return -1;

	/* Check if stdin */
	if (fd == STDIN_FILE)
		return -1;
		
	/* Check if file descriptor has been initialized */
	if (current_pcb->file_array[fd].flags == FREE_)
		return -1;
		
	return current_pcb->file_array[fd].ops->write(fd, buf, nbytes);//term_write(fd,buf,nbytes)
}

/* int32_t syscall_open(const uint8_t* filename)
*	Inputs: const uint8_t* filename = name of the file
*	Return Value: 0 upon successful close, otherwise -1
*	Function: Open the file
*/
int32_t syscall_open(const uint8_t* filename) {
	dentry_t temp;
	int32_t fd;
	int32_t i;
	
	/* Checks whether file exists */
	if (read_dentry_by_name(filename, &temp) == -1)
		return -4;
		
	/* Clear file arrays */
	for (i = REGULAR_FILE_START; i < MAX_FILES; i++) {
		current_pcb->file_array[i].ops = 0;
		current_pcb->file_array[i].inode_num = -1;
		current_pcb->file_array[i].file_pos = 0;
		current_pcb->file_array[i].flags = 0;
	}
	
	/* Find unused file descriptor */
	for (fd = REGULAR_FILE_START; fd < MAX_FILES+1; fd++) {
		/* Check for end of file array */
		if (fd == MAX_FILES)
			return -3;
			
		/* Found unused file descriptor */
		if (current_pcb->file_array[fd].flags == FREE_);
			break;
	}
	
	/* Setup and call respective open functions based on file type */
	switch(temp.f_type) {
		case TYPE_RTC:
				current_pcb->file_array[fd].ops = &rtc_ops;
				rtc_open(fd);
				break;
		case TYPE_DIR:
				current_pcb->file_array[fd].ops = &dir_ops;
				dir_open(fd);
				break;
		case TYPE_FILE:
				current_pcb->file_array[fd].ops = &file_ops;
				current_pcb->file_array[fd].inode_num = temp.inode_num;
				file_open(fd);
				break;
		default:
				current_pcb->file_array[fd].ops = &stdin_ops;
				term_open(fd);
				break;
	}
	
	/* Initialize stdin and stdout */
	init_stds(current_pcb);
	
	return 0;
}

/* int32_t syscall_close(int32_t fd)
*	Inputs: int32_t fd = file descriptor
*	Return Value: 0 upon successful close, otherwise -1
*	Function: Close the file
*/
int32_t syscall_close(int32_t fd) {
	/* Check for stdin/stdout fd */
	if (fd == (STDIN_FILE || STDOUT_FILE)) {
		return -1;
	}
	
	/* Check if file is actually open */
	if(current_pcb->file_array[fd].flags == FREE_) {
		return -1;
	}
	
	/* Reset/erase data in file descriptor */
	current_pcb->file_array[fd].ops = NULL;
	current_pcb->file_array[fd].inode_num = -1;
	current_pcb->file_array[fd].file_pos = NULL;
	current_pcb->file_array[fd].flags = FREE_;
	
	return 0;
}

void init_stds(pcb_t* cur_pcb) {
	/* Initialize stdin of current PCB's file array */
	cur_pcb->file_array[STDIN_FILE].ops = &stdin_ops;
	cur_pcb->file_array[STDIN_FILE].inode_num = -1;
	cur_pcb->file_array[STDIN_FILE].file_pos = 0;
	cur_pcb->file_array[STDIN_FILE].flags = IN_USE;
	
	/* Initialize stdout of current PCB's file array */
	cur_pcb->file_array[STDOUT_FILE].ops = &stdout_ops;
	cur_pcb->file_array[STDOUT_FILE].inode_num = -1;
	cur_pcb->file_array[STDOUT_FILE].file_pos = 0;
	cur_pcb->file_array[STDOUT_FILE].flags = IN_USE;
}

/*
 * getargs
 *   DESCRIPTION: Reads command line arguments into user-level buffer
 *   INPUTS: uint8_t* buf - buffer to write into, int32_t nbytes - length of buffer
 *   OUTPUTS: none
 */
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
	/* Check for non-existent buff */
	if (buf == NULL)
		return -1;

	/* Get arguments from PCB */
	uint8_t* args = current_pcb->arg;
	
	/* Get length of arguments */
	uint32_t length = strlen((int8_t*)args);
	
	/* Check if arguments + '\0' fit in buffer */
	/* The args array already holds '\0' */
	if(length > nbytes)
		return -1;
		
	/* Else, read  arguments into buffer */
	/* If not arguments, will just rest '\0' */
	int i;
	for(i = 0; i < length; i++)
		buf[i] = args[i];
	
	return 0;
}

/*
 * vidmap
 *   DESCRIPTION: Maps text-mode video memory into user space
 *   INPUTS: uint8_t** screen_start - pointer to address of video memory virtual address
 *   OUTPUTS: none (address stored in location from screen_start)
 */
int32_t vidmap (uint8_t** screen_start)
{
	/* Check if valid pointer
	 * input is a pointer to a pointer which points to video memory virtual address
	 * we need to make sure the first pointer is not NULL
	 * it should point to a location where we'll put the address in
	 */
	if(screen_start == NULL)
		return -1;
		
	/* Don't need to do much validation for user-pointer for this MP
	 * Visible area of the screen uses less that 4 kB so don't need more space 
	 */

	/* Need page to map first 4kB starting from VIDEO into a 4 kB page at VID_VIRT_ADDR
	 * This is done in page_init.c
	 */
	
	/* Make screen_start point to video memory virtual address */
	*screen_start = (uint8_t*)VID_VIRT_ADDR;
	
	return 0;
}

