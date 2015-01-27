#ifndef _SYSCALLHANDLER_H
#define _SYSCALLHANDLER_H

#include "idt.h"

extern int32_t syscall_halt (uint8_t status);
extern int32_t syscall_execute (const uint8_t* command);
extern int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t syscall_open (const uint8_t* filename);
extern int32_t syscall_close (int32_t fd);

void system_call(void);

#endif
