/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "handlers.h"
#include "keyboard.h"
#include "rtc.h"
#include "common_interrupt.h"
#include "test_int_handlers.h"
#include "terminal.h"
#include "paging.h"
#include "filesystem.h"
#include "file_descriptor.h"
#include "tests.h"
#include "process.h"
#include "soundblaster.h"
#include "colors.h"
#include "signal.h"
#include "mouse.h"

#define MOUSE_IRQ 12
#define KEYBOARD_IRQ 1
#define RTC_IRQ 8
#define USER_IRQ_OFFSET 32
#define SYS_CALL_IDT_ENTRY 0x80
#define NUM_IRQS 15
#define VIDEO 0xB8000
#define STARTUP_SCREEN_COLORS 0xF0


/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

#define SIZE_OF_IDT_ENTRY 8 //8 bytes per idt entry

//global vars
extern volatile int soundcard_is_open;

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
	volatile unsigned long i = 0;
	//int8_t testbuffer[128];

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
		tss.ss0 = KERNEL_DS; //segment where the kernel stack is located
		tss.esp0 = 0x800000; //kernel stack base
		ltr(KERNEL_TSS);
	}
	/*initalize IDTR with proper location*/
	lidt(idt_desc_ptr);						/*set IDTR with argument as defined in x86_desc.S*/
	
	/*set up IDT entries for handlers*/
	for(i = 0; i<32; i++)
		SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[i], unkexcepthandler); //set all idt entries for system exceptions as unknown first
	/*set individual, expected handler entries*/
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[0], handler0);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[3], handler3);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[4], handler4);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[5], handler5);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[6], handler6);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[7], handler7);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[8], handler8);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[9], handler9);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[10], handler10);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[11], handler11);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[12], handler12);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[13], handler13);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[14], handler14_linkage);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[16], handler16);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[17], handler17);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[18], handler18);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[19], handler19);
	
	for(i = 32; i<NUM_VEC;i++)
		SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[i], unkwn_interrupt); //set all idt entries for external interrupts
											
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[(KEYBOARD_IRQ+USER_IRQ_OFFSET)], kbd_interrupt);//set up specific handlers for the keyboard/rtc/pit/mosue
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[(RTC_IRQ+USER_IRQ_OFFSET)], rtc_interrupt);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[(PIT_IRQ+USER_IRQ_OFFSET)], pit_interrupt);
	SET_IDT_ENTRY(KERNEL_CS, KERNEL_DPL, idt[(MOUSE_IRQ+USER_IRQ_OFFSET)], mouse_interrupt);
	
	SET_IDT_ENTRY(KERNEL_CS, USER_DPL, idt[SYS_CALL_IDT_ENTRY], syscallwrapper); // set up handler for system calls
	
	
	/* Init the PIC */
	i8259_init();
	for (i = 0; i < NUM_IRQS; i++)
		disable_irq(i);

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */

	// filesystem initialization
	module_t* filesystem_mod = (module_t*) mbi->mods_addr;
	filesystem_init((boot_block_t*) filesystem_mod->mod_start); 

	//terminal initalization
	terminal_init();
	
	//mouse initalization
	mouse_init();
	
	//rtc initalization
	rtc_init();

	
	clear();
	reset_printf_pos();
	printf("\nGroup07 OS - Booting...\n\n\n\n\n");
	//ascii art from http://www.oracle.com/javaone/lad-en/session-presentations/corejava/22641-enok-1439101.pdf
	printf("          0000_____________0000________0000000000000000__000000000000000000\n");
	printf("        00000000_________00000000______000000000000000__0000000000000000000\n");
	printf("       000____000_______000____000_____000_______0000__00______0\n");
	printf("      000______000_____000______000_____________0000___00______0\n");
	printf("     0000______0000___0000______0000___________0000_____0_____0\n");
	printf("     0000______0000___0000______0000__________0000___________0\n");
	printf("     0000______0000___0000______0000_________000+__0000000000\n");
	printf("     0000______0000___0000______0000________0000\n");
	printf("      000______000_____000______000________0000\n");
	printf("       000____000_______000____000_______00000\n");
	printf("        00000000_________00000000_______0000000\n");
	printf("          0000_____________0000________000000007\n");
	
	
	uint8_t tempbuf[NUM_ROWS*NUM_COLS];
	uint8_t startbuf[NUM_ROWS*NUM_COLS];
	
	change_colors(startbuf);
	

	//change to Bond Colors
	for(i = 0; i < NUM_ROWS*NUM_COLS; i++)
		tempbuf[i] = STARTUP_SCREEN_COLORS;
	change_colors(tempbuf);
	

	startup_beep(); //GODDAMN THIS THING IS ANNOYING BUT IT'S AWESOME!!!!
	change_colors(startbuf);
	clear();
	
	soundcard_is_open = 0; //initalize the soundcard as unused

	//Scheduler initialization
	sched_init();
	
	//Signal initialization
	signals_init();
	

	/* Enable interrupts */	
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	//printf("Enabling Interrupts\n");
	//sti(); 
	
	// I'm intentionally setting up paging at the end.
	// The reason is that the multiboot information structure may be placed
	// *anywhere* in memory, so we could easily page fault while accessing it.
	// By this point, we should be done with the structure, so we can get away
	// with just mapping the kernel and the VGA.
	map_kernel_vga_pages();
	map_user_vga_page(VIDEO);
	map_kernel_page();
	enable_paging();

	
	// disable that hardware goddamned cursor
	outb(0x0a, 0x3d4);
	outb(0x20, 0x3d5); //Chris: Sweetness! I couldn't find the right commands to do it. Thanks whomever this was!
	

	/* Execute the first program (`shell') ... */
	start_initial_shells(); 
	
	//should never hit this point in the code....
	
	
	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

