/* rtc_handler.h - Defines for function to initialize page directories,
* page tables, and pages
*/

#ifndef _RTC_HANDLER_H
#define _RTC_HANDLER_H

#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "idt_entry_handler.h"

#define RTC_IRQ 8
#define PIC_CASCADE_IRQ 2
#define RTC_ADDR_PORT 0x70
#define RTC_DATA_PORT 0x71
#define ENABLE_PERIODIC_INPUT 0x40
#define REG_C 0x0C
#define REG_B_MASK_NMI 0x8B
#define REG_A_MASK_NMI 0x8A
#define WRITE_MASK 0xF0
#define RTC_DEFAULT 2

void rtc_handler(void);
void initialize_rtc(void);

int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open (int32_t fd);
int32_t rtc_close (int32_t fd);

unsigned char rate_to_arg(uint32_t rtc_rate);

extern volatile int rtc_interrupted;

#endif
