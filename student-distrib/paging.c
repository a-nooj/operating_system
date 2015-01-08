/* paging.c - structures and functions related to paging */

#include "paging.h"

/* Paging-related constants */
#define KB_PAGE_OFFSET_BITS 12
#define MB_PAGE_OFFSET_BITS 22
#define NUM_PTE 1024 // number of entries in page directory/page table

#define VGA_START_PAGE (0xb8000 >> KB_PAGE_OFFSET_BITS)
#define VGA_END_PAGE   (0xc0000 >> KB_PAGE_OFFSET_BITS)
#define KERNEL_ADDR    0x400000
#define PROCESS_V_ADDR 0x8000000

/* Page directories and tables */
// these will be zero-initialized automatically
static pde_t page_directory[NUM_PTE] __attribute__((aligned(0x1000)));
// maps the first 4 MB of memory
static pte_t page_table_0[NUM_PTE] __attribute__((aligned(0x1000)));

/* Local functions - see headers for details */
static void
map_large_page(uint32_t virtual_addr, uint32_t physical_addr, int is_kernel);
static void flush_tlb();


/*
 * map_kernel_vga_pages
 *   DESCRIPTION: Maps kernel VGA pages to the correct part of physical memory.
 *                Video memory is at the same location in virtual memory
 *                as it is in physical memory.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets the first PDE and some entries in the first page table.
 */
void map_kernel_vga_pages(void) {
	pde_t pde;
	pde.val = 0; // clear all bits out
	pde.kb.present = 1; // don't want page fault
	pde.kb.read_write = 1; // want to write to VGA
	pde.kb.user_supervisor = 1; // user VGA page can be accessed in user mode
	pde.kb.page_size = 0; // first 4 MB divided into 4 kB pages
	pde.kb.table_addr = (uint32_t) page_table_0 >> 12;
	page_directory[0] = pde;

	pte_t pte;
	pte.val = 0;
	pte.present = 1;
	pte.read_write = 1;
	pte.global = 1;
	// I believe caching is disabled globally so this may be unneeded
	pte.cache_disabled = 1;
	uint32_t vga_page;
	for (vga_page = VGA_START_PAGE; vga_page < VGA_END_PAGE; ++vga_page) {
		pte.page_addr = vga_page;
		// indexing by vga_page only works since upper 10 bits of address are 0
		page_table_0[vga_page] = pte;
	}
	// the other page table entries (including the one corresponding to NULL)
	// are left as 0 and will hence trigger page faults on access
}


/*
 * map_user_vga_page
 *   DESCRIPTION: Maps the user VGA page to the specified address.
 *                In text mode, the screen size is 80 * 25 * 2 = 4000 bytes,
 *                which is just under one page.
 *   INPUTS: addr -- the address to map to.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets some PTEs and flushes the TLB.
 */
void map_user_vga_page(uint32_t addr) {
	pte_t pte;
	pte.val = 0;
	pte.present = 1;
	pte.read_write = 1;
	pte.user_supervisor = 1;
	pte.global = 1;
	// I believe caching is disabled globally so this may be unneeded
	pte.cache_disabled = 1;
	pte.page_addr = addr >> KB_PAGE_OFFSET_BITS;

	page_table_0[USER_VGA_ADDR >> KB_PAGE_OFFSET_BITS] = pte;
	flush_tlb();
}


/*
 * map_kernel_page
 *   DESCRIPTION: Maps the kernel page to the correct part of physical memory.
 *                This is at the same location in virtual memory
 *                as it is in physical memory.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets the second PDE to map to the kernel.
 */
void map_kernel_page(void) {
	map_large_page(KERNEL_ADDR, KERNEL_ADDR, 1);
}


/*
 * map_process_page
 *   DESCRIPTION: Maps a process page to the correct part of physical memory.
 *                A process' virtual memory is always at 128 MB
 *                and its physical memory at either 8 or 12 MB.
 *   INPUTS: addr -- the physical address of the process' memory.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets the appropriate PDE to map to the process.
 *                 flushes the TLB (except global pages).
 */
void map_process_page(uint32_t addr) {
	map_large_page(PROCESS_V_ADDR, addr, 0);
	flush_tlb();
}


/*
 * enable_paging
 *   DESCRIPTION: Enables paging.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to CR0, CR3 and CR4.
 */
void enable_paging(void) {
	asm volatile(
			"movl	%%cr4, %%eax\n\t"
			"andl	$~0x20, %%eax	# disable PAE\n\t"
			"orl    $0x90, %%eax	# enable global pages and large pages\n\t"
			"movl	%%eax, %%cr4\n\t"
			"movl	%0, %%cr3		# set the page directory base register\n\t"
			"movl	%%cr0, %%eax\n\t"
			"orl	$1<<31, %%eax	# enable paging\n\t"
			"movl	%%eax, %%cr0"
			: 
			: "r" (page_directory)
			: "eax", "memory"
			);
}


/*
 * flush_tlb
 *   DESCRIPTION: Flushes the TLB.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: writes to CR3.
 */
static void flush_tlb() {
	asm volatile(
		"movl	%%cr3, %%eax\n\t"
		"movl	%%eax, %%cr3"
		:
		:
		: "eax", "memory"
	);
}


/*
 * map_large_page
 *   DESCRIPTION: Maps a large (4 MB) page from virtual to physical memory.
 *   INPUTS: virtual_addr -- the virtual address of the page.
 *           physical_addr -- the physical address it maps to.
 *           is_kernel -- whether the page belongs to the kernel
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets the PDE corresponding to the virtual address.
 */
static void
map_large_page(uint32_t virtual_addr, uint32_t physical_addr, int is_kernel) {
	pde_t pde;
	pde.val = 0;
	pde.mb.present = 1;
	pde.mb.read_write = 1;
	pde.mb.user_supervisor = !is_kernel;
	pde.mb.page_size = 1;
	pde.mb.global = is_kernel;
	pde.mb.page_addr = physical_addr >> MB_PAGE_OFFSET_BITS;
	page_directory[virtual_addr >> MB_PAGE_OFFSET_BITS] = pde;
}
