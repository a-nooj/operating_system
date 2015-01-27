/* page_init.c - Holds function to initialize 1 page directory
* and 1 page table upon bootup
*/

#include "page_init.h"

int p_directory[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
int p_table[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
int process0_dir[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
int process1_dir[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));
int vid_mem[PAGE_ENTRIES] __attribute__((aligned (PAGE_ALIGN)));

void page_init() {
	int i;
	int start_addr;

	/* Allow RW for page directory */
	for(i=0; i < PAGE_ENTRIES; i++) {
		p_directory[i] = RW_FLAG;
		process0_dir[i] = RW_FLAG;
		process1_dir[i] = RW_FLAG;
	}

	/* Mapping first 4MB of memory by 4KB pages into page table */
	for(i = 0, start_addr = 0; i < PAGE_ENTRIES; i++, start_addr += PAGE_ALIGN) {
		if(start_addr == 0) {
			p_table[i] = 0;
			continue;
		}

		p_table[i] = start_addr | P_FLAG | RW_FLAG;
	}

	vid_mem[(VID_MEM_VIRTUAL << 10) >> PD_SHIFT] = VID_MEM_ADDR | P_FLAG | US_FLAG | RW_FLAG;
	
	/* Map page table into page directory */
	p_directory[0] = (int) p_table | P_FLAG | US_FLAG | RW_FLAG; //| G_FLAG
	p_directory[KERNEL_ADDR >> PD_SHIFT] = KERNEL_ADDR | P_FLAG | RW_FLAG | PD_PS_FLAG | G_FLAG;
	p_directory[PROG_VIRT_ADDR>>PD_SHIFT] = PROCESS0_PHYS_ADDR;
	p_directory[USER_STACK>>PD_SHIFT] = PROCESS1_PHYS_ADDR;
	process0_dir[0] = (int) p_table | P_FLAG | US_FLAG | RW_FLAG | G_FLAG;
	process1_dir[1] = (int) p_table | P_FLAG | US_FLAG | RW_FLAG | G_FLAG; 

	p_directory[PROG_VIRT_ADDR >> PD_SHIFT] = PROCESS0_PHYS_ADDR | P_FLAG | PD_PS_FLAG | RW_FLAG | G_FLAG;
	p_directory[VID_MEM_VIRTUAL >> PD_SHIFT] = (int) vid_mem | US_FLAG | RW_FLAG | P_FLAG;
	

	process0_dir[0] = (int) p_table | P_FLAG | US_FLAG | RW_FLAG | G_FLAG;
	process1_dir[0] = (int) p_table | P_FLAG | US_FLAG | RW_FLAG | G_FLAG;
	
	/* Map process directories to shared kernel */
	process0_dir[KERNEL_ADDR >> PD_SHIFT] = KERNEL_ADDR | P_FLAG | RW_FLAG | PD_PS_FLAG | G_FLAG;
	process1_dir[KERNEL_ADDR >> PD_SHIFT] = KERNEL_ADDR | P_FLAG | RW_FLAG | PD_PS_FLAG | G_FLAG;
	
	/* Map process directories from virtual 128MB to physical 8MB and 12 MB */
	process0_dir[PROG_VIRT_ADDR >> PD_SHIFT] = PROCESS0_PHYS_ADDR | P_FLAG | PD_PS_FLAG | RW_FLAG | US_FLAG;
	process1_dir[PROG_VIRT_ADDR >> PD_SHIFT] = PROCESS1_PHYS_ADDR | P_FLAG | PD_PS_FLAG | RW_FLAG;

	/* Map video memory from virtual 132MB to physical 736 KB */
	process0_dir[VID_MEM_VIRTUAL >> PD_SHIFT] = (int) vid_mem | US_FLAG | RW_FLAG | P_FLAG;
	process1_dir[VID_MEM_VIRTUAL >> PD_SHIFT] = (int) vid_mem | US_FLAG | RW_FLAG | P_FLAG;
	
	/* Enable 4MB page access */
	asm volatile("movl %%cr4, %%eax\n\t"
	"movl %0, %%ebx\n\t"
	"orl %%ebx, %%eax\n\t"
	"movl %%eax, %%cr4"
	: /* no outputs */
	: "r" (CR4_PSE_FLAG) /* inputs */
	: "%eax", "%ebx" /* clobber list */
	);

	/* Load address of page directory into CR3 */
	asm volatile("movl %0, %%cr3"
	: /* no outputs */
	: "r" (p_directory) /* inputs */
	);

	/* Enable paging by setting PG and PE flags in CR0 */
	asm volatile("movl %%cr0, %%eax\n\t"
	"movl %0, %%ebx\n\t"
	"orl %%ebx, %%eax\n\t"
	"movl %1, %%ebx\n\t"
	"orl %%ebx, %%eax\n\t"
	"movl %%eax, %%cr0"
	: /* no outputs */
	: "r" (CR0_PG_FLAG), "r" (CR0_PE_FLAG) /* inputs */
	: "%eax", "%ebx" /* clobber list */
	);
}

