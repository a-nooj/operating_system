#include "idt.h"

void set_idt_struct(uint32_t idt_entry_num, uint32_t handler_func_ptr) {

	idt[idt_entry_num].reserved0 = DISABLE;
	idt[idt_entry_num].reserved1 = ENABLE;
	idt[idt_entry_num].reserved2 = ENABLE;
	idt[idt_entry_num].reserved4 = DISABLE;
	idt[idt_entry_num].size = ENABLE;

	if (idt_entry_num == IDT_SYSCALL_INDEX) { //sys call
		idt[idt_entry_num].dpl = USER_PRIV;
		idt[idt_entry_num].reserved3 = DISABLE;
	}
	
	else {
		idt[idt_entry_num].dpl = KERNEL_PRIV;
		idt[idt_entry_num].reserved3 = ENABLE;
	}
	
	idt[idt_entry_num].seg_selector = KERNEL_CS;
	idt[idt_entry_num].present = ENABLE;
	
	SET_IDT_ENTRY(idt[idt_entry_num], handler_func_ptr);
}

void initialize_idt() {
	set_idt_struct(0, (uint32_t) &divide_error);
	set_idt_struct(1, (uint32_t) &debug);
	set_idt_struct(2, (uint32_t) &nmi);
	set_idt_struct(3, (uint32_t) &int3);
	set_idt_struct(4, (uint32_t) &overflow);
	set_idt_struct(5, (uint32_t) &bounds);
	set_idt_struct(6, (uint32_t) &invalid_op);
	set_idt_struct(7, (uint32_t) &device_not_available);
	set_idt_struct(8, (uint32_t) &doublefault_fn);
	set_idt_struct(9, (uint32_t) &coprocessor_segment_overrun);
	set_idt_struct(10, (uint32_t) &invalid_tss);
	set_idt_struct(11, (uint32_t) &segment_not_present);
	set_idt_struct(12, (uint32_t) &stack_segment);
	set_idt_struct(13, (uint32_t) &general_protection);
	set_idt_struct(14, (uint32_t) &page_fault);
	set_idt_struct(15, (uint32_t) &reserved_by_intel);
	set_idt_struct(16, (uint32_t) &coprocessor_error);
	set_idt_struct(17, (uint32_t) &alignment_check);
	set_idt_struct(18, (uint32_t) &machine_check);
	set_idt_struct(19, (uint32_t) &simd_coprocessor_error);
	set_idt_struct(20, (uint32_t) &reserved_by_intel);
	set_idt_struct(21, (uint32_t) &reserved_by_intel);
	set_idt_struct(22, (uint32_t) &reserved_by_intel);
	set_idt_struct(23, (uint32_t) &reserved_by_intel);
	set_idt_struct(24, (uint32_t) &reserved_by_intel);
	set_idt_struct(25, (uint32_t) &reserved_by_intel);
	set_idt_struct(26, (uint32_t) &reserved_by_intel);
	set_idt_struct(27, (uint32_t) &reserved_by_intel);
	set_idt_struct(28, (uint32_t) &reserved_by_intel);
	set_idt_struct(29, (uint32_t) &reserved_by_intel);
	set_idt_struct(30, (uint32_t) &reserved_by_intel);
	set_idt_struct(31, (uint32_t) &reserved_by_intel);

	//set_idt_struct(33, (uint32_t) &keyboard_interrupt);
	set_idt_struct(40, (uint32_t) &rtc_interrupt);

	set_idt_struct(33, (uint32_t) &keyboard_interrupt);
	set_idt_struct(40, (uint32_t) &rtc_interrupt);

	set_idt_struct(128, (uint32_t) &system_call);
}
