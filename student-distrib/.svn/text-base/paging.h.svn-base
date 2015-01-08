/* paging.h - prototypes for various paging-related functions */

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

/* The user virtual VGA address */
#define USER_VGA_ADDR 0x98000

/* Maps kernel VGA pages to the correct addresses */
void map_kernel_vga_pages(void);

/* Maps the user VGA page to the specified address */
void map_user_vga_page(uint32_t addr);

/* Maps the kernel page to the correct address */
void map_kernel_page(void);

/* Maps a process page to the correct address */
void map_process_page(uint32_t addr);

/* Enables paging */
void enable_paging(void);

/* Paging structures */

// A page directory entry (goes into a page directory).
// kb is for 4 kB entries and mb for 4 MB entries.
// There's a lot of duplication betwen the two and
// page table entries, and I could perhaps do clever
// things with anonymous structs and unions to avoid
// that, but I don't think it's worth the headache.
typedef union pde_t {
	uint32_t val;
	struct {
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t reserved : 1;
		uint32_t page_size : 1;
		uint32_t global : 1; // ignored for 4 kB pages
		uint32_t avail : 3;
		uint32_t table_addr : 20;
	} __attribute__((packed)) kb;
	struct {
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t dirty : 1;
		uint32_t page_size : 1;
		uint32_t global : 1;
		uint32_t avail : 3;
		uint32_t pat_index : 1;
		uint32_t reserved : 9;
		uint32_t page_addr : 10;
	} __attribute__((packed)) mb;
} pde_t;

// A page table entry (goes into a page table)
typedef union pte_t {
	uint32_t val;
	struct {
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t dirty : 1;
		uint32_t pat_index : 1;
		uint32_t global : 1;
		uint32_t avail : 3;
		uint32_t page_addr : 20;
	} __attribute__((packed));
} pte_t;


#endif
