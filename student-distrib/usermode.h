/* user_mode.c - Defines function to execute user-level code
 * with context switching
 */
 
#ifndef _USER_MODE_H
#define _USER_MODE_H

#include "x86_desc.h"
#include "syscall.h"

#ifndef ASM

#define USER_STACK_PTR 0x083FFFFC//0x08400000-4//0x007FFFFC//// 132 MB in hex (128 MB + 4 MB page)

void context_switch(unsigned int entry);
void modify_tss(int32_t old_esp);

#endif
#endif

