/* signal.h - signal related prototypes */

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "types.h"
#include "lib.h"
#include "process.h"

#define DIV_ZERO 0
#define SEGFAULT 1
#define INTERRUPT 2
#define ALARM 3
#define USER1 4

#define KILL_TASK 0
#define IGNORE_SIG 1

#define NUM_SIGS 5

/* sets the handler for a signal */
extern int32_t set_handler(int32_t signum, void* handler_address);

/* returns from a signal */
extern int32_t sigreturn(void);

//Sends a signal to a process
extern void send_signal(int signum, int proc_id);

//Handle pending signals on switching into a process.
extern void handle_pending_signals (int proc_id);

//Handle the pending signals for the current process
extern void handle_current_pending_signals ();

//Clear the signals for proc_id
extern void clear_signals(int proc_id);

//initalize signals
void signals_init();

#endif
