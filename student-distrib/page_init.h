/* page_init.h - Defines for function to initialize page directories,
* page tables, and pages
*/

#ifndef _PAGE_INIT_H
#define _PAGE_INIT_H

#include "types.h"
#include "syscall.h"

#define KERNEL_ADDR	 0x00400000	// Starting address of the kernel in memory
#define PAGE_ALIGN	 0x00001000	// 4K = 2^2 + 2^10 = 2^12
#define PAGE_ENTRIES 1024	 // Number of entries for page directory and page tables
#define PD_SHIFT 22				// Number of shifts to right to get 10 bit offset for page directory
#define VID_MEM_ADDR 0x000B8000	// Starting address of video memory in physical address
#define VID_MEM_VIRTUAL 0x08400000	// Virtual address 132MB

#define CR0_PG_FLAG	 0x80000000	// Bit 31 enabling PG flag to enable paging
#define CR0_PE_FLAG	 0x00000001	// Bit 1 switches processor to protected mode
#define CR4_PSE_FLAG 0x00000010	// Bit 4 setting PSE flag to enable 4MB page access

#define G_FLAG	 	 0x00000100	// Bit 8 set to indicate global page
#define PD_PS_FLAG	 0x00000080	// Bit 7 (page size) in page directory to set 4MB page
#define A_FLAG	 	 0x00000020	// Bit 5 indicate page(table) accessed when set
#define PCD_FLAG	 0x00000010	// Bit 4 set to prevent caching of associated page(table)
#define PWT_FLAG	 0x00000008	// Bit 3 set to enable write-through caching
#define US_FLAG		 0x00000004	// Bit 2 set to assign user-level privilege, clear for supervisor
#define RW_FLAG		 0x00000002	// Bit 1 set to specify read-write privileges
#define P_FLAG		 0x00000001	// Bit 0 of page directory/table entries to signal present

void page_init();

extern int p_directory[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
extern int p_table[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
extern int process0_dir[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
extern int process1_dir[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));

#endif

