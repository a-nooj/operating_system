/*sched.h - header for all functions associated with scheduler*/
#ifndef _SCHED_H
#define _SCHED_H

#define EPOCH_LENGTH 20 // (in ms)
#define PIT_FRQ 18 //Frequency of the PIT in Hz
#define MS_PER_SECONDS 1000
#define EPOCH_TICKS (EPOCH_LENGTH * PIT_FRQ) / MS_PER_SECONDS
#define PIT_IRQ 0

#include "lib.h"
#include "process.h"

//Global Functions
//This function figures out if an epoch has ended and will cause a process switch if it has
extern void schedule_tasks(void);

//This function intializes the scheduler
extern void sched_init();

//Local Functions
//This fuction finds the PID of the new process in a round-robin way
int find_new_child_process();

#endif
