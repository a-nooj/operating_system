/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"

#include "kb.h"
#include "rtc.h"
#include "file_system.h"
#include "idt.h"
#include "page_init.h"
#include "syscall.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		
		/* Assign pointer of file system module to global variable */
		fs_boot = (boot_block_t*) mod->mod_start;
		
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}
	
	
	//uncomment to test filesystem
	
	// Filesys test code STARTS HERE!
/*	
	uint8_t buf[40000];
	uint8_t fname[NAME_LEN] = "shell";//"frame1.txt"; // Modify file name here!!!
	uint8_t flist[NAME_LEN];
	dentry_t temp;
	uint32_t i, file_length, bytes_read;

	read_dentry_by_name(fname, &temp);
	uint32_t* inode_addr = (uint32_t*)((uint32_t)fs_boot + (temp.inode_num + 1) * BLOCK_SIZE);
	file_length = *inode_addr;
//	bytes_read = file_read(fname, buf, file_length); // Change file_length to desired length if needed
	bytes_read = read_data(temp.inode_num, 0, buf, file_length);

	printf("Testing file_read\n"); // testing file_read
	printf("file name: ");
	for (i = 0; i < NAME_LEN; i++) {
		printf("%c", fname[i]);
	}

	printf("file length: %d\n", bytes_read);
	
	// Uncomment below to print file data
	printf("file data: \n");
	for (i = 0; i < 400; i++) {
		printf("%c", buf[i]);
	}
*/

/*	
	printf("\nTesting dir_read\n"); // testing dir_read
	dir_read(flist, NAME_LEN);
	printf("d_entry 0: ");
	for (i = 0; i < NAME_LEN; i++) {
		printf("%c", flist[i]);
	}
	dir_read(flist, NAME_LEN);
	printf("\nd_entry 1: ");
	for (i = 0; i < NAME_LEN; i++) {
		printf("%c", flist[i]);
	}
	dir_read(flist, NAME_LEN);
	printf("\nd_entry 2: ");
	for (i = 0; i < NAME_LEN; i++) {
		printf("%c", flist[i]);
	}
	dir_read(flist, NAME_LEN);
	printf("\nd_entry 3: ");
	for (i = 0; i < NAME_LEN; i++) {
		printf("%c", flist[i]);
	}
	dir_read(flist, NAME_LEN);
	printf("\nd_entry 4: ");
	for (i = 0; i < NAME_LEN; i++) {
	printf("%c", flist[i]);
	}
*/	
	// Filesys test code ENDS HERE!
		
	/* Init the IDT */
	initialize_idt();

	/* Init the PIC */
	i8259_init();
	
	/* Init paging*/
	page_init();

	/* Init the keyboard driver */
	//initialize_keyboard();

	/* Init the RTC driver */
	initialize_rtc();
	
	sti();
/*	
	uint8_t namme[32] = "shell";
	uint32_t* namme_addr = namme;
	asm volatile("movl $5, %%eax\n\t"
			"movl %0, %%ebx\n\t"
			"call syscall_open\n\t"
			:
			: "g"(namme_addr)
			);
	
	printf("\npass system open\n");
*/
	//run_shell();
	
	/* Checks for Paging */
/*	int *p = (int*) 0x00c00001;
	*p = 2;
*/

	//int *p = (int*) 0x00900000;
	//*p = 2;
	
	//uncomment to test RTC
	/*
	int ret = 0;
	int32_t int2 = 0;
	rtc_open(NULL);
	
	int32_t int5 = 5;
	ret = rtc_write(int2, &int5, int2);
	printf("\n status: %d \n", ret);
	
	int hi;
	for(hi = 0; hi<10; hi++) {
		rtc_read(int2, NULL, int2);
		printf("two");
	}
	
	int5 = 4;
	ret = rtc_write(int2, &int5, int2);
	printf("\n status: %d \n", ret);

	for(hi = 0; hi<10; hi++) {
		rtc_read(int2, NULL, int2);
		printf("four");
	}

	rtc_close(int2);
*/

	//uncomment to test terminal
/*	term_open(NULL);

	int len;
	uint8_t buf[128];

	int j=0;
	while(j<50) {
		term_write(-1,"\nEnter test input(unlimited):\n", 29);
		len = term_read(-1,buf, 128);
		term_write(-1,(char*)buf, len);
		j++;
	}
*/

		
	//asm volatile("INT $40");

	//check for divide by 0 exception
	//uint8_t a = 8;
	//a = 8/0;

	//check for page fault
/*	int *p = (int*) 0x00900000;
	*p = 2;
*/
	
	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	/*printf("Enabling Interrupts\n");
	sti();*/

	/* Execute the first program (`shell') ... */
	int8_t *fname = "shell\0";
	asm volatile("movl $2, %%eax; movl %0, %%ebx; int $0x80"
			:
			: "g"(fname)
			:"memory", "%eax" // clobber list
			);
			
	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

