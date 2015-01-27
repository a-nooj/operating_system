#include "idt_entry_handler.h"

volatile int rtc_interrupted;

void divide_error() {
	printf("EXCEPTION: Division by Zero\n");
	while(1);
}

void debug() {
	printf("EXCEPTION: Debug\n");
	while(1);
}

void nmi() {
	printf("EXCEPTION: Non-Maskable Interrupt\n");
	while(1);
}

void int3() {
	printf("EXCEPTION: Breakpoint\n");
	while(1);
}

void overflow() {
	printf("EXCEPTION: Overflow\n");
	while(1);
}

void bounds() {
	printf("EXCEPTION: Bounds Exceeded\n");
	while(1);
}

void invalid_op() {
	printf("EXCEPTION: Invalid Opcode\n");
	while(1);
}

void device_not_available() {
	printf("EXCEPTION: Device not Available\n");
	while(1);
}

void doublefault_fn() {
	printf("EXCEPTION: Double Fault\n");
	while(1);
}

void coprocessor_segment_overrun() {
	printf("EXCEPTION: Coprocessor Segment Overrun\n");
	while(1);
}

void invalid_tss() {
	printf("EXCEPTION: Invalid TSS\n");
	while(1);
}

void segment_not_present() {
	printf("EXCEPTION: Segment not Present\n");
	while(1);
}

void stack_segment() {
	printf("EXCEPTION: Stack Segment Fault\n");
	while(1);
}

void general_protection(uint32_t EIP, uint32_t error_code) {
	uint32_t fault_address;
	printf("EXCEPTION: General Protection\nEIP:0x%x  error code:%d",EIP,error_code);
	asm volatile("mov %%cr2, %0":"=r" (fault_address));
	printf("\nFault address: 0x%x",fault_address);
	while(1);
}

void page_fault(uint32_t EIP, uint32_t error_code) {
	uint32_t fault_address;
	printf("EXCEPTION: Page Fault\nEIP:0x%x  error code:%d",EIP,error_code);
	asm volatile("mov %%cr2, %0":"=r" (fault_address));
	printf("\nFault address: 0x%x",fault_address);
	while(1);
}

void coprocessor_error() {
	printf("EXCEPTION: Floating-Point Error\n");
	while(1);
}

void alignment_check() {
	printf("EXCEPTION: Alignment Check\n");
	while(1);
}

void machine_check() {
	printf("EXCEPTION: Machine Check\n");
	while(1);
}

void simd_coprocessor_error() {
	printf("EXCEPTION: SIMD Floating-Point Exception\n");
	while(1);
}

void reserved_by_intel() {
	printf("Reserved by Intel\n");
	while(1);
}

void keyboard_interrupt() {
	kb_handler();
}

void rtc_interrupt() {
	rtc_interrupted = 1;
	rtc_handler();
}
