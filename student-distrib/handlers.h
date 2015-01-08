/*handler.h - contains functions for exception handling and interrupt handling.*/

#include "x86_desc.h"
#include "lib.h"
#include "debug.h" 
#include "i8259.h"
#include "keyboard.h"
#include "terminal.h"
#include "rtc.h"
#include "sched.h"
//#include "colors.h"
#include "process.h"

//system call definitions
#define HALT 1
#define EXECUTE 2
#define READ 3
#define WRITE 4
#define OPEN 5
#define CLOSE 6
#define GETARGS 7
#define VIDMAP 8
#define SET_HANDLER 9
#define SIGRETURN 10
#define CHANGE_COLORS 11


//define fixed File Descriptors
#define STDIN 0
#define STDOUT 1
#define STDERR 2


/*local function definitions*/
void unkexcepthandler(unsigned long firstarg); //handler for exceptions which we don't expect to have to handle
/*Handlers for each system exception*/
void handler0(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler3(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler4(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler5(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler6(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler7(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler8(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler9(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler10(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler11(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler12(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler13(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler14(unsigned long, unsigned long);
void handler16(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler17(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler18(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
void handler19(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);

/*handlers for interrupts*/
extern void unkinthandler(void);

extern void kbd_handler(void);
extern void irq2_handler(void);
extern void rtc_handler(void);
extern void pit_handler(void);

/*functions to associate another function with an IRQ line*/
extern int request_irq( void (*handler)(void), unsigned int IRQline);
extern int free_irq(unsigned int IRQline);

