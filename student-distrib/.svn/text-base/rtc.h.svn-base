/*rtc.h - Includes all functions associated with the rtc initalization and rtc driver*/
#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "file_descriptor.h"

#define RTC_IRQ 8
#define SLAVE_IRQ 2
#define RTC_REG_PORT 0x70
#define RTC_DATA_PORT 0x71
#define REG_A_MASK_NMI 0x8A
#define REG_B_MASK_NMI 0x8B
#define RTC_DEFAULT_RATE_DATA 0x2F //2 Hz
#define RTC_MAX_RATE_DATA 0x26
#define PERIODIC_INPUT_ENABLE 0x40

#define MAX_PROCESSES 6
#define RTC_MIN_RATE 1
#define RTC_MAX_RATE 1024

/*function definitions*/
void rtc_init(void);
int32_t rtc_open(void);
int32_t rtc_close(void);
unsigned char RTC_rate_to_byte(uint32_t rate);

extern int32_t rtc_read(file_desc_t* file_desc, void* buf, int32_t nbytes);
extern int32_t rtc_write(file_desc_t* file_desc, const void* buf, int32_t nbytes);

/* Global variables */
extern volatile int32_t rtc_is_open [MAX_PROCESSES+1]; // zero-initialized automatically
extern volatile int32_t rtc_interrupt_ocurred; //Set to 1 when the RTC interrupt occurs
extern volatile uint32_t rtc_count;

#endif

