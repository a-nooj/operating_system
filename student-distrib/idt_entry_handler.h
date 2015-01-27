//#include "keyboard.h"
#include "kb.h"
#include "lib.h"
#include "rtc.h"

void divide_error();
void debug();
void nmi();
void int3();
void overflow();
void bounds();
void invalid_op();
void device_not_available();
void doublefault_fn();
void coprocessor_segment_overrun();
void invalid_tss();
void segment_not_present();
void stack_segment();
void general_protection();
void page_fault();
void coprocessor_error();
void alignment_check();
void machine_check();
void simd_coprocessor_error();
void reserved_by_intel();


void keyboard_interrupt();
void rtc_interrupt();
