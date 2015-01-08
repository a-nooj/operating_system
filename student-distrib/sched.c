#include "sched.h"


//Global vars
static int ticks; //ticks into the current epoch
extern PCB * ProcessPCBs[MAX_PROCESSES+1];
extern volatile int testprint_pending_flag;

/* schedule_tasks
 * DESCRIPTION: times the length of the epochs, and switches between the currently running processes if the time budget has ended
 * INPUTS: none
 * OUTPUTS: Changes the curerntly running process...maybe...
 * RETURN VAL: none
 * NOTES: 
 */
void schedule_tasks(void){
	if ( ticks < EPOCH_TICKS ) {
		ticks++; //Epoch continues
		printk("ticks: %d", ticks);
		return;
	}
	else {
		int newPID = find_new_child_process();
		ticks = 0;
		if (newPID != current_process) {
			switch_process(current_process, newPID);
		}
	}
}

/* find_new_child_process
 * DESCRIPTION: goes through all processes, finds currently running processes, and returns the next one to run
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VAL: PID of new process to switch to
 * NOTES: relies on the PCB structs
 */

int find_new_child_process() {
	int i;
	int newPID = (current_process+1) % (MAX_PROCESSES+1);
	unsigned char PIDrunning[MAX_PROCESSES + 1]; //array of 1's/0's saying which processes are currently running (eg, has no children)
	
	//use existing data structures to deduce which processes are currently running
	for(i = 0; i < (MAX_PROCESSES + 1); i++) //make runningPIDs array same as in-use array to start
		PIDrunning[i] = PIDinUse[i];
		
	for(i = 1; i < (MAX_PROCESSES + 1); i++) //set all parents as not running, only children run.
	{
		if(PIDinUse[i] != 0)
			PIDrunning[ProcessPCBs[i]->parent_process] = 0;
	}
	
	//select the next running process
	for (i = 0; i < (MAX_PROCESSES + 1); i++) {
		//iterate over all the PID's, starting with the current one, until we find the next running one.
		if ((PIDrunning[newPID] != 0)  && (newPID != 0)){ //&& !has_child_process(newPID)
			return newPID;
		}
		newPID = (newPID + 1) % (MAX_PROCESSES + 1);
	}
	return -1;
}

/* sched_init
 * DESCRIPTION: sets up the PIT to do scheduling
 * INPUTS: none
 * OUTPUTS: enables IRQ's
 * RETURN VAL: none
 * NOTES: 
 */

void sched_init() {
	ticks = 0; //Start a new epoch
	request_irq(&schedule_tasks, PIT_IRQ); //bind the scheduler to the PIT interrupt
	enable_irq(PIT_IRQ); //Enable PIT
}




