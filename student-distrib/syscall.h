/* syscall.h - Defines for system call funcitons
*
*/

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "syscall_handler.h"
#include "types.h"
#include "file_system.h"
#include "rtc.h"
#include "kb.h"
#include "page_init.h"
#include "usermode.h"
#include "x86_desc.h"
#include "lib.h"

/* General */
#define MAX_FILES 8
#define STDIN_FILE 0
#define REGULAR_FILE_START 2
#define STDOUT_FILE 1
#define MAX_PROCESSES 2
#define IN_USE 1
#define FREE_ 0
#define STACK_SIZE 8192
#define VID_VIRT_ADDR 0x08400000	// virtual address for video memory

/* Magic Numbers */
#define INITIAL_BYTE 0x7F
#define E 0x45
#define L 0x4C
#define F 0x46

/* Program Image */
#define PROG_IMG_ADDR 0x08048000
#define PROG_VIRT_ADDR 0x08000000 // Virtual memory 128MB
#define OFFSET 0x00048000
#define USER_STACK 0x08400000 // PROG_VIRT_ADDR + 4MB

#define PROC0 0
#define PROC1 1

#define FLAGS 0x246

/* Process 0 */
#define PROCESS0_PHYS_ADDR 0x00800000 // Physical address 8MB
#define PROCESS0_KERNEL_STACK 0x00800000
#define PROCESS0_PCB (pcb_t*)0x007FE000 // Physical address 8MB - 8KB
#define PROCESS0_OFFSET_ADDR (PROCESS0_PHYS_ADDR + OFFSET) // Offset within page for copy of program image

/* Process 1 */
#define PROCESS1_PHYS_ADDR 0x00C00000 // Physical address 12MB
#define PROCESS1_KERNEL_STACK 0x007FE000 // Physical address 8KB above bottom of 4MB
#define PROCESS1_PCB (pcb_t*)0x007FC000 // Physical address 8MB - 16KB
#define PROCESS1_OFFSET_ADDR (PROCESS1_PHYS_ADDR + OFFSET) // Offset within page for copy of program image


/* Operations Table */
typedef struct ops_t {
	int32_t (*open)(int32_t fd);
	int32_t (*close) (int32_t fd);
	int32_t (*read)(int32_t fd, void* buf, int32_t length);
	int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} ops_t;

/* File Descriptor struct */
typedef struct file_desc_t{
	ops_t* ops;
	uint32_t inode_num;
	uint32_t file_pos;
	uint32_t flags;
}file_desc_t;

typedef struct pcb_t {
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	
	uint32_t pid;
	uint32_t* p_dir;
	file_desc_t file_array[MAX_FILES];
	uint8_t arg[BUFFER_SIZE];
	struct pcb_t* parent_process;
} pcb_t;

extern pcb_t* current_pcb;
extern pcb_t* process0_pcb;
extern pcb_t* process1_pcb;

extern ops_t file_ops;
extern ops_t dir_ops;
extern ops_t rtc_ops;
extern ops_t stdin_ops;
extern ops_t stdout_ops;

/* System Calls */
int32_t syscall_halt(uint8_t status);
int32_t syscall_execute(const uint8_t* command);
int32_t syscall_read(int32_t fd, void* buf, int32_t nbytes);
int32_t syscall_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t syscall_open(const uint8_t* filename);
int32_t syscall_close(int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** screen_start);
int32_t run_shell();

/* Helper Functions */
void init_stds(pcb_t* cur_pcb);
void set_tss();

#endif
