#include "idt_entry_handler.h"
#include "x86_desc.h"
#include "types.h"
#include "syscall_handler.h"

#define IDT_SYSCALL_INDEX 0x80 
#define USER_PRIV 3
#define KERNEL_PRIV 0
#define DISABLE 0
#define ENABLE 1

void set_idt_struct(uint32_t idt_entry_num, uint32_t handler_func_ptr);
void initialize_idt();
