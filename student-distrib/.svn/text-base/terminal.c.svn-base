/* terminal.c - Functions used by the terminal to display the screen and process read/write/open/close requests
 * vim:ts=4 noexpandtab
 */
 
#include "terminal.h"
#include "process.h"
#include "mouse.h"

extern PCB * ProcessPCBs[MAX_PROCESSES+1]; //reference the process pcbs from elsewhere (process.c)
extern volatile int ls_pending_flag;
extern volatile int shortcut_pending_flag;
extern volatile char executeable_name[32];
extern volatile int executable_name_length;

//set up video memory location
#define VIDEO 0xB8000
#define VIDEOMEMSIZE 4096 // just above 2 * NUM_ROWS * NUM_COLS, needs to be a multiple of page size
static char* video_mem_screen = (char *)VIDEO;
static char* video_mems[3] = {(char *)(VIDEO+VIDEOMEMSIZE*1), (char *)(VIDEO+VIDEOMEMSIZE*2), (char *)(VIDEO+VIDEOMEMSIZE*3)};
 
extern volatile int mouse_x;
extern volatile int mouse_y;
extern char old_attribute;
 
//data struct for keeping track of the three terminals
terminal_data terms[MAX_NUM_TERMS];

//global variable telling the currently displayed/active terminal
int current_terminal;
//global variable of PID's, showing which is the curerntly executing program on each terminal.
//for example, if terminal 0 has fish running, the array would contain the PID of fish in index 0, and the PID's of the other two shells in indices 1 and 2
extern int top_level_PID[MAX_NUM_TERMS];



/* terminal_kbd_handler
 * DESCRIPTION: To be attached to kbd IRQ line. This function decodes the keyboard keycodes, prints out approprate characters to the screen (based on 
 * 				modifier keys), and if a read() function is running, dumps approprate characters into read's buffer. Additionally, every keystroke is logged 
 *				into terms[current_terminal].inputbuf for future reference - probably needed for terminal switching in mp3.5. Also, used to correctly backspace through
 *				newline characters.
 * INPUTS: none
 *
 * OUTPUTS: read()'s buffer if requested, inputbuf are filled with keystrokes
 * RETURN VAL: nothing. Resets "read_in_progress" bit if enter is hit or max characters in buffer is met.
 * NOTES: called from interrupt context, so usual no-blocking-no-sleeping rules apply
 */
void terminal_kbd_handler(void)
{
	unsigned char LastKey;
	int xiter = 0;
	int yiter = 0;
	int shiftstate = 0; //one if either of the ctrl/alt/shift keys are pressed
	int ctrlstate = 0;
	int altstate = 0;
	int read_done_flag = 0;

	LastKey = inb(KEYBOARD_PORT);//read in the byte we got interrupted for
	cli(); //ensure interrupts are disabled
	shiftstate = terms[current_terminal].lshift | terms[current_terminal].rshift; //or works b/c the two states are defined as 1 for pressed, 0 for up. 
	ctrlstate = terms[current_terminal].lctrl | terms[current_terminal].rctrl;	  //each state should be 1 only if either or both of the keys are pressed (eg, either is 1)
	altstate = terms[current_terminal].lalt | terms[current_terminal].ralt;
	
	//disable mouse pointer during write to prevent curser multiplication bug
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute;
	
	//handle ctrl-L case
	if(keyfull(LastKey, shiftstate) == 'l' && ctrlstate == KEY_DOWN)
	{
		clear(); //clear off screen
		terms[current_terminal].xpos = 0; //set position to upper left corner
		terms[current_terminal].ypos = 0;
		terms[current_terminal].last_x_written = 0; //no writes have been performed yet
		terms[current_terminal].last_y_written = 0;
	}

	//CTRL-k case
	else if(keyfull(LastKey, shiftstate) == 'k' && ctrlstate == KEY_DOWN
			&& current_process != 1 && current_process != 2 && current_process != 3)
	{	if (current_process == terms[current_terminal].process_on_this_term) {
			printk("WARNING: HARD_KILL on process %d!\n", current_process);	
			halt(100); //signal abnormal interrupt
		}
	}
	
	//CTRL-C case
	else if(keyfull(LastKey, shiftstate) == 'c' && ctrlstate == KEY_DOWN ) //&& current_process != 1 && current_process != 2 && current_process != 3)
	{	
		send_signal(INTERRUPT, terms[current_terminal].process_on_this_term);
	}
	
	
	//handle normal keypress - echo to screen, move curser, and put character into buffer
	// make sure we only allow keyboard presses to be echoed to the screen/input buffers if a read is in progress
	else if((keyfull(LastKey, shiftstate) != 0x00) && terms[current_terminal].read_in_progress == TRUE)
	{		
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = keyfull(LastKey, shiftstate);
        *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CHARATTRIB;

		


		if(terms[current_terminal].readbuf_index < (terms[current_terminal].readbuf_size - 1)) //only write into the first readbuf_size-1 locations
			terms[current_terminal].readbuf[terms[current_terminal].readbuf_index++] = keyfull(LastKey, shiftstate);
		else 
			terms[current_terminal].readbuf_index++; //however, do keep track of how many keystrokes have been hit. this is required for backspace to work properly


		
		//move to the next spot
		terms[current_terminal].xpos++;
		
	}
	else if(((LastKey == BACKSPACE) && !((terms[current_terminal].last_x_written == terms[current_terminal].xpos) && (terms[current_terminal].last_y_written == terms[current_terminal].ypos))) && terms[current_terminal].read_in_progress == TRUE) 
	{
		//handle backspace
		//elseif condition dissallows backspace to do anything if we're where the write() function left off
		//that way, we can't backspace through stuff that the system wrote to the screen		
		
		//clear out previous character
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = BLANK_CHAR;
        *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CHARATTRIB;
		

		terms[current_terminal].xpos--;

		
		//if we've got a read() system call in progress
		if(terms[current_terminal].read_in_progress == TRUE)
		{
			if(terms[current_terminal].readbuf_index < (terms[current_terminal].readbuf_size - 1)) // if we're within the buffer,
			{
				//decrement the buffer index, but not if the decrement would make it negative
				if(terms[current_terminal].readbuf_index > 0)
				{
					terms[current_terminal].readbuf_index--;
				}
				terms[current_terminal].readbuf[terms[current_terminal].readbuf_index] = 0x00; //clear out the current index in the buffer
			}
			else
				terms[current_terminal].readbuf_index--; //if we're not within the buffer, just decrement the pointer
		}

		
	}
	//handle modifier keys by changing status bits. All terminals are affected by a status bit change, so there arent' any problems with a stuck
	//alt or shift key on a terminal switch
	else if(LastKey == LEFT_SHIFT_PRESS)
	{
		//try to fix hardware bug - PS2 keyboard only sends shift keycodes if the other key is released
		terms[0].lshift = KEY_DOWN;
		terms[0].rshift = KEY_UP; 
		terms[1].lshift = KEY_DOWN;
		terms[1].rshift = KEY_UP; 
		terms[2].lshift = KEY_DOWN;
		terms[2].rshift = KEY_UP; 
	}
	else if(LastKey == LEFT_SHIFT_RELEASE)
	{
		terms[0].lshift = KEY_UP;
		terms[0].rshift = KEY_UP;
		terms[1].lshift = KEY_UP;
		terms[1].rshift = KEY_UP;
		terms[2].lshift = KEY_UP;
		terms[2].rshift = KEY_UP;
		
	}
	else if(LastKey == RIGHT_SHIFT_PRESS)
	{
		terms[0].rshift = KEY_DOWN;
		terms[0].lshift = KEY_UP;
		terms[1].rshift = KEY_DOWN;
		terms[1].lshift = KEY_UP;
		terms[2].rshift = KEY_DOWN;
		terms[2].lshift = KEY_UP;
	}
	else if(LastKey == RIGHT_SHIFT_RELEASE)
	{
		terms[0].rshift = KEY_UP;
		terms[0].lshift = KEY_UP;
		terms[1].rshift = KEY_UP;
		terms[1].lshift = KEY_UP;
		terms[2].rshift = KEY_UP;
		terms[2].lshift = KEY_UP;
	}
	else if(LastKey == LEFT_CTRL_PRESS)
	{
		terms[0].lctrl = KEY_DOWN;
		terms[1].lctrl = KEY_DOWN;
		terms[2].lctrl = KEY_DOWN;
	}
	else if(LastKey == LEFT_CTRL_RELEASE)
	{
		terms[0].lctrl = KEY_UP;
		terms[1].lctrl = KEY_UP;
		terms[2].lctrl = KEY_UP;
	}
	else if(LastKey == LEFT_ALT_PRESS)
	{
		terms[0].lalt = KEY_DOWN;
		terms[1].lalt = KEY_DOWN;
		terms[2].lalt = KEY_DOWN;
	}
	else if(LastKey == LEFT_ALT_RELEASE)
	{
		terms[0].lalt = KEY_UP;
		terms[1].lalt = KEY_UP;
		terms[2].lalt = KEY_UP;
	}
	else if(LastKey == EXTENDED_KEYCODE) //handle extended keycode for right ctlr and alt
	{
		LastKey = inb(KEYBOARD_PORT); //read in next byte, as this will actually contain the keycode we want
		if(LastKey == RIGHT_CTRL_PRESS)
		{
			terms[0].rctrl = KEY_DOWN;
			terms[1].rctrl = KEY_DOWN;
			terms[2].rctrl = KEY_DOWN;
		}
		else if(LastKey == RIGHT_CTRL_RELEASE)
		{
			terms[0].rctrl = KEY_UP;
			terms[1].rctrl = KEY_UP;
			terms[2].rctrl = KEY_UP;
		}
		else if(LastKey == RIGHT_ALT_PRESS)
		{
			terms[0].ralt = KEY_DOWN;
			terms[1].ralt = KEY_DOWN;
			terms[2].ralt = KEY_DOWN;
		}
		else if(LastKey == RIGHT_ALT_RELEASE)
		{
			terms[0].ralt = KEY_UP;
			terms[1].ralt = KEY_UP;
			terms[2].ralt = KEY_UP;
		}	
	
	}
	//handle enter presses
	else if(LastKey == ENTER_PRESSED && terms[current_terminal].read_in_progress == TRUE)
	{

		//clear out curser
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = BLANK_CHAR;
        *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CHARATTRIB;
		
		
		//go to new line
		terms[current_terminal].xpos = 0;
		terms[current_terminal].ypos++;

		
		read_done_flag = 1;
		
	
	}
	else if(LastKey == F1_PRESS && altstate != 0 && current_terminal != 0) //handle switch to terminal 0
	{
		cli();
		(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute; //restore attribute
		terms[current_terminal].video_mem_start = video_mems[current_terminal]; //set current process to write to its own storage buffer
		switch_video_memory(current_terminal, 0); //change out the screen data
		terms[0].video_mem_start = video_mem_screen; //change process 0's video memory to point to the active region for the screen
		map_user_vga_page((uint32_t) terms[ProcessPCBs[current_process]->terminal_num].video_mem_start);
		current_terminal = 0; //switch current terminal to 0
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;
	}
	else if(LastKey == F2_PRESS && altstate != 0 && current_terminal != 1) //handle switch to terminal 1
	{
		
		cli();
		(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute; //restore attribute
		terms[current_terminal].video_mem_start = video_mems[current_terminal]; //set current process to write to its own storage buffer
		switch_video_memory(current_terminal, 1); //change out the screen data
		terms[1].video_mem_start = video_mem_screen; //change process 1's video memory to point to the active region for the screen
		map_user_vga_page((uint32_t) terms[ProcessPCBs[current_process]->terminal_num].video_mem_start);
		current_terminal = 1; //switch current terminal to 1
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;
	}
	else if(LastKey == F3_PRESS && altstate != 0 && current_terminal != 2) //handle switch to terminal 2
	{
		cli();
		(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute; //restore attribute
		terms[current_terminal].video_mem_start = video_mems[current_terminal]; //set current process to write to its own storage buffer
		switch_video_memory(current_terminal, 2); //change out the screen data
		terms[2].video_mem_start = video_mem_screen; //change process 2's video memory to point to the active region for the screen
		map_user_vga_page((uint32_t) terms[ProcessPCBs[current_process]->terminal_num].video_mem_start);
		current_terminal = 2; //switch current terminal to 2
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;
	}
	
	

	//fix current position if it goes off the screen

	//if we went to before the begining of a line b/c of backspace, reset to the line above
	if(terms[current_terminal].xpos < 0)
	{
		terms[current_terminal].xpos = (terms[current_terminal].xmax)-1; //subtract 1 because max is total number of col's, and we use 0-based indexing
		terms[current_terminal].ypos--;
	}
	//corner case - curser is at start of terminal, don't move it
	if(terms[current_terminal].ypos < 0)
	{
		terms[current_terminal].xpos = 0;
		terms[current_terminal].ypos = 0;
	}
	//if we've hit the end if the line, go to next line
	if(terms[current_terminal].xpos >= terms[current_terminal].xmax)
	{
		terms[current_terminal].xpos = 0;
		terms[current_terminal].ypos++;
	}
	//if we've gone over the edge of the bottom of the screen, scroll by moving everything up by one line in video memory
	if(terms[current_terminal].ypos >= terms[current_terminal].ymax)
	{
		//clear out curser
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = BLANK_CHAR;
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CHARATTRIB;
		//copy everything up by one line
		for(yiter = 0; yiter < (terms[current_terminal].ymax-1); yiter++)
		{
			for(xiter = 0; xiter < (terms[current_terminal].xmax); xiter++)
			{
				*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*yiter + xiter) << 1)) = *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*(yiter+1) + xiter) << 1));
				*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*yiter + xiter) << 1) + 1) = *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*(yiter+1) + xiter) << 1) + 1);
			}
		}
		//clear out bottom row (new and clean line to type in!)
		for(xiter = 0; xiter < terms[current_terminal].xmax; xiter++)
		{
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*(terms[current_terminal].ymax - 1) + xiter) << 1)) = BLANK_CHAR;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*(terms[current_terminal].ymax - 1) + xiter) << 1) + 1) = CHARATTRIB;
		}
			
		terms[current_terminal].ypos = (terms[current_terminal].ymax)-1; //reset current location to proper location
	}
	
	//print out current terminal number and into upper right corner
	switch(current_terminal){
	
		case 0:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '0';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM0ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		case 1:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '1';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM1ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		case 2:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '2';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM2ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		default:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '?';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = UNKTERMATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		
	}
	
	//set new curser
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = CURSER_CHAR;
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CURSERATTRIB;
	
	//reinable mouse pointer
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = CURSER_ATTRIBUTE;

	if(read_done_flag)
	{
		read_done_flag = 0;
		//signal read() that it can return
		terms[current_terminal].read_in_progress = FALSE;
	}
}

/* terminal_init
 * DESCRIPTION: Performs all actions required to initalize the terminal
 * INPUTS: none
 *
 * OUTPUTS: initalizes the terms[current_terminal] data sctructure
 * RETURN VAL: currently 0 on success
 * NOTES: should be called by kernel during startup to initalize the terminal
 */
int32_t terminal_init(void)
{
	unsigned long flags;
	int i, j;
	
	cli_and_save(flags); //disable interrupts while initalizing
	//clear the screen to start
	clear();
	
	for(i = 0; i < MAX_NUM_TERMS; i++)
	{
		//initalize terms[current_terminal] data structure
		terms[i].lalt = KEY_UP;
		terms[i].lshift = KEY_UP;
		terms[i].lctrl = KEY_UP;
		terms[i].xpos = 0;
		terms[i].ypos = 0;
		terms[i].xmax = NUM_COLS;
		terms[i].ymax = NUM_ROWS;
		terms[i].video_mem_start = video_mems[i];
		terms[i].last_x_written = 0;
		terms[i].last_y_written = 0;
		
		//initalize read() system call variables
		terms[i].read_in_progress = FALSE; //not doing a read yet...
		//terms[i].readbuf = NULL; //just in case we access it, cause a system exception right away.
		for(j= 0; j < READ_BUF_MAX; j++)
			terms[i].readbuf[j] = 0x00;
		terms[i].readbuf_index = 0;
		terms[i].readbuf_size = READ_BUF_MAX;
		terms[i].process_on_this_term = i+1;//NOTE: THIS SETS UP THE PIDS BEFORE THE SHELLS ACTUALLY START - 
											//COULD CAUSE PROBLEMS IF LOTS OF FUNCTIONS ARE INSERTED BETWEEN INIT_TERM and START_INITAL_SHELLS
	}
	
		for(j= 0; j < READ_BUF_MAX; j++)
			terms[i].readbuf[j] = 0x00;
	
	
	
	//initalize keyboard (enables irq line on pic for keyboard)
	keyboard_init();
	
	//attach terminal handler to keyboard IRQ line
	request_irq(&terminal_kbd_handler, KEYBOARD_IRQ);
	
	//restore the flags and interrupts 
	restore_flags(flags);
	
	//set up term0 as the inital one
	current_terminal = 0;
	terms[current_terminal].video_mem_start = video_mem_screen; //set up address such that writes will go to the screen
	
	//set up initial curser and report successful opening
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1)) = CURSER_CHAR;
    *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*terms[current_terminal].ypos + terms[current_terminal].xpos) << 1) + 1) = CURSERATTRIB;
	return 0;


}
/* terminal_read
 * DESCRIPTION: Sets the read flag and waits for the user to press enter or for the input buffer to be filled, and then returns user keystrokes
 * 				in the specified buffer
 * INPUTS: buf - buffer into which we need to place the user's keystrokes. not null terminated, although unused spaces will be 0x00
 *		   nbytes - size of input buffer in bytes. this is how read knows when to return, in the event that the buffer has been filled before
 *					enter was pressed.
 * OUTPUTS: none
 * RETURN VAL: number of bytes read in
 * NOTES: behavior is to fill up the input buffer with, at most, the first nbytes-1 keystroke it gets, and then return a newline-terminated buffer
 *        after enter is pressed.
 */
int32_t terminal_read(void * buf, int32_t nbytes)
{
	int i = 0;
	int j = 0;
	int caller_terminal = ProcessPCBs[current_process]->terminal_num;
	int caller_process = current_process;
	char * user_buf = (char * ) buf;
	terms[caller_terminal].readbuf_index = 0;
	//terms[caller_terminal].readbuf = (unsigned char *)buf;
	//terms[caller_terminal].readbuf_size = nbytes;
	//clear out readbuf first to fix weird bug with sched
	for(i = 0; i < terms[caller_terminal].readbuf_size; i++) 
		terms[caller_terminal].readbuf[i] = 0x00;
	terms[caller_terminal].read_in_progress = TRUE; //set flag saying we should put keyboard characters into the read buffer
								   //this flag is reset to false by the keyboard handler when enter is pressed
	
	while(terms[caller_terminal].read_in_progress == TRUE || caller_terminal != current_terminal)
	{
		sti();
		//if we've got a shortcut pending, artifically insert the command for execution
		if(ls_pending_flag && caller_terminal == current_terminal)
		{
			cli();
			terms[caller_terminal].read_in_progress = FALSE;
			terms[caller_terminal].readbuf[0] = 'l';
		    terms[caller_terminal].readbuf[1] = 's';	
			terms[caller_terminal].readbuf_index = 2;
			ls_pending_flag = 0;
		}
		else if(shortcut_pending_flag && caller_terminal == current_terminal)
		{
			cli();
			terms[caller_terminal].read_in_progress = FALSE;
			for(j = 0; j < executable_name_length; j++) //copy contents over
				terms[caller_terminal].readbuf[j] = executeable_name[j];
			terms[caller_terminal].readbuf_index = executable_name_length;
			shortcut_pending_flag = 0;
		}
	
	} 
	//wait until enter is pressed
	//while we sit in this while loop, the interrupt handler is filling the buffer with keystrokes
	//or handle shortcuts being accessed

	if(terms[caller_terminal].readbuf_index < (terms[caller_terminal].readbuf_size - 2)) // if we're within the buffer,
	{
		terms[caller_terminal].readbuf[terms[caller_terminal].readbuf_index++] = '\n'; //insert newline character into last written index
		terms[caller_terminal].readbuf[terms[caller_terminal].readbuf_index++] = '\0'; //null-terminate the string
		
		
		do{		//wait for the desired process to be remapped until we write everything back to userland
			sti();
			cli();
		}while(current_process != caller_process);
		for(i = 0; i < nbytes && i < terms[caller_terminal].readbuf_index; i++)
			user_buf[i] = terms[caller_terminal].readbuf[i];		
		
		terms[caller_terminal].read_in_progress = FALSE;//make sure flag got reset
		sti();
		return terms[caller_terminal].readbuf_index-1; //return total bytes read in from kbd (not including null character)
	}
	else //if we overran the buffer with the number of keystrokes
	{
		terms[caller_terminal].readbuf[terms[caller_terminal].readbuf_size-2] = '\n'; //insert newline into the second-to-last available slot
		terms[caller_terminal].readbuf[terms[caller_terminal].readbuf_size-1] = '\0'; //null-terminate string
		
		do{		//wait for the desired process to be remapped until we write everything back to userland
			sti();
			cli();
		}while(current_process != caller_process);
		for(i = 0; i < nbytes && i < (terms[caller_terminal].readbuf_size - 1); i++)
			user_buf[i] = terms[caller_terminal].readbuf[i];		
		
		terms[caller_terminal].read_in_progress = FALSE;//make sure flag got reset
		sti();
		return terms[caller_terminal].readbuf_size-1; //return max bytes read in
	}

}

/* terminal_write
 * DESCRIPTION: takes a output buffer, and writes its contents to the screen
 * INPUTS: buf - buffer from which to read the character to be put to the screen.
 *		   nbytes - size of input buffer in bytes. this is how we know how many characters to write to the screen.
 * OUTPUTS: none
 * RETURN VAL: number of bytes read in
 * NOTES: these characters are also written to the internal buffer for the terminal to keep track of what was on the screen
 */

int32_t terminal_write(const void * buf, int32_t nbytes)
{
	int i = 0;
	int xiter = 0;
	int yiter = 0;
	unsigned char * input = NULL;
	int caller_terminal = 0;
	
	unsigned long flags;
	cli_and_save(flags); //disable interrupts while writing
	caller_terminal = ProcessPCBs[current_process]->terminal_num; //set which terminal we're writing to (might not be to the screen)
	
	input = (unsigned char *)buf; //typecast the input
	
	//disable mouse pointer during write to prevent curser multiplication bug
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute;

	for(i = 0; i < nbytes; i++)
	{
		
		//write character to screen
		if(input[i] == '\n') //go to new line if it's a newline char
		{
			//clear out curser
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ypos) + terms[caller_terminal].xpos) << 1)) = BLANK_CHAR;
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ypos) + terms[caller_terminal].xpos) << 1) + 1) = CHARATTRIB;
			//move to new line
			terms[caller_terminal].xpos = 0;
			terms[caller_terminal].ypos++;
		}
		else
		{
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1)) = input[i];
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1) + 1) = CHARATTRIB;
			terms[caller_terminal].xpos++;
		}
		
		
		//if we've hit the end if the line, go to next line
		if(terms[caller_terminal].xpos >= terms[caller_terminal].xmax)
		{
			terms[caller_terminal].xpos = 0;
			terms[caller_terminal].ypos++;
		}
		
		//if we've gone over the edge of the bottom of the screen, scroll by moving everything up by one line in video memory
		if(terms[caller_terminal].ypos >= terms[caller_terminal].ymax)
		{
			//copy everything up by one line
			for(yiter = 0; yiter < (terms[caller_terminal].ymax-1); yiter++)
			{
				for(xiter = 0; xiter < (terms[caller_terminal].xmax); xiter++)
				{
					*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*yiter + xiter) << 1)) = *(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(yiter+1) + xiter) << 1));
					*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*yiter + xiter) << 1) + 1) = *(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(yiter+1) + xiter) << 1) + 1);
				}
			}
			//clear out bottom row (new and clean line to type in!)
			for(xiter = 0; xiter < terms[caller_terminal].xmax; xiter++)
			{
				*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ymax - 1) + xiter) << 1)) = BLANK_CHAR;
				*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ymax - 1) + xiter) << 1) + 1) = CHARATTRIB;
			}
				
			terms[caller_terminal].ypos = (terms[caller_terminal].ymax)-1; //reset current location to proper location
		}
	}
	
	//set new curser
	*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1)) = CURSER_CHAR;
	*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1) + 1) = CURSERATTRIB;
	
	//mark where the write call left off, so the user won't backspace over system written stuff
	terms[caller_terminal].last_x_written = terms[caller_terminal].xpos;
	terms[caller_terminal].last_y_written = terms[caller_terminal].ypos;
	
	//print out current terminal number and into upper right corner
	switch(current_terminal){
	
		case 0:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '0';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM0ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		case 1:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '1';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM1ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		case 2:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '2';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = TERM2ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		default:
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1)) = '?';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1) << 1) + 1) = UNKTERMATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1)) = '>';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*1) << 1) + 1) = TERM_SWITCH_ATTRIB;
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1)) = 'M';
			*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax-1+terms[current_terminal].xmax*2) << 1) + 1) = TERM_SWITCH_ATTRIB;
		break;
		
	}
	
	//reinable mouse pointer
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = CURSER_ATTRIBUTE;
	
	//restore the flags and interrupts 
	restore_flags(flags);
	
	//return number of bytes written
	return nbytes;
}



/* terminal_write_kernel
 * DESCRIPTION: takes a output buffer, and writes its contents to a particular terminal
 * INPUTS: buf - buffer from which to read the character to be put to the screen.
 *		   nbytes - size of input buffer in bytes. this is how we know how many characters to write to the screen.
 *		   term_num - the terminal to write to
 * OUTPUTS: none
 * RETURN VAL: number of bytes read in
 * NOTES: these characters are also written to the internal buffer for the terminal to keep track of what was on the screen
 */

int32_t terminal_write_kernel(const void * buf, int32_t nbytes, int caller_terminal)
{
	int i = 0;
	int xiter = 0;
	int yiter = 0;
	unsigned char * input = NULL;
	
	unsigned long flags;
	cli_and_save(flags); //disable interrupts while writing
	
	input = (unsigned char *)buf; //typecast the input
	
	for(i = 0; i < nbytes; i++)
	{
		
		//write character to screen
		if(input[i] == '\n') //go to new line if it's a newline char
		{
			//clear out curser
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ypos) + terms[caller_terminal].xpos) << 1)) = BLANK_CHAR;
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ypos) + terms[caller_terminal].xpos) << 1) + 1) = CHARATTRIB;
			//move to new line
			terms[caller_terminal].xpos = 0;
			terms[caller_terminal].ypos++;
		}
		else
		{
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1)) = input[i];
			*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1) + 1) = CHARATTRIB;
			terms[caller_terminal].xpos++;
		}
		
		
		//if we've hit the end if the line, go to next line
		if(terms[caller_terminal].xpos >= terms[caller_terminal].xmax)
		{
			terms[caller_terminal].xpos = 0;
			terms[caller_terminal].ypos++;
		}
		
		//if we've gone over the edge of the bottom of the screen, scroll by moving everything up by one line in video memory
		if(terms[caller_terminal].ypos >= terms[caller_terminal].ymax)
		{
			//copy everything up by one line
			for(yiter = 0; yiter < (terms[caller_terminal].ymax-1); yiter++)
			{
				for(xiter = 0; xiter < (terms[caller_terminal].xmax); xiter++)
				{
					*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*yiter + xiter) << 1)) = *(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(yiter+1) + xiter) << 1));
					*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*yiter + xiter) << 1) + 1) = *(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(yiter+1) + xiter) << 1) + 1);
				}
			}
			//clear out bottom row (new and clean line to type in!)
			for(xiter = 0; xiter < terms[caller_terminal].xmax; xiter++)
			{
				*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ymax - 1) + xiter) << 1)) = BLANK_CHAR;
				*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*(terms[caller_terminal].ymax - 1) + xiter) << 1) + 1) = CHARATTRIB;
			}
				
			terms[caller_terminal].ypos = (terms[caller_terminal].ymax)-1; //reset current location to proper location
		}
	}
	
	//set new curser
	*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1)) = CURSER_CHAR;
	*(uint8_t *)(terms[caller_terminal].video_mem_start + ((terms[caller_terminal].xmax*terms[caller_terminal].ypos + terms[caller_terminal].xpos) << 1) + 1) = CURSERATTRIB;
	
	//mark where the write call left off, so the user won't backspace over system written stuff
	terms[caller_terminal].last_x_written = terms[caller_terminal].xpos;
	terms[caller_terminal].last_y_written = terms[caller_terminal].ypos;

	//restore the flags and interrupts 
	restore_flags(flags);
	
	//return number of bytes written
	return nbytes;
}



//DEFUNCT, UNUSED FUNCTION
int32_t terminal_close(void)
{
	/*Do nothing because this is an invalid action*/
	// Shoaib: I'm not using this; perhaps it could be removed?
	// Chris: I left these in for the sake of grading - the gradsheet said we needed functions like this, even if they return nothing
	//			I dont' think we'll need them, but I threw them in just in case the TA's asked to see our close() function for the terminal
	return -1;
}
//DEFUNCT, UNUSED FUNCTION
int32_t terminal_open(void)
{
	/*Do nothing because this is an invalid action*/
	// Shoaib: I'm not using this; perhaps it could be removed?
	return -1;
}


//wrapper functions to make terminal_read and terminal_write conform to standard function prototypes
//also perform some basic error checking
int32_t stdin_read(file_desc_t* file_desc, void* buf, int32_t nbytes) {
	// terminal_read should maybe be modified to take extra unused parameter
	// it depends on if anyone ever needs to use it directly
	if (nbytes <= 0) {
		// returns -1 for nbytes < 0 and 0 for nbytes == 0
		return -(nbytes < 0);
	}
	return terminal_read(buf, nbytes);
}

int32_t stdout_write(file_desc_t* file_desc, const void* buf, int32_t nbytes) {
	// terminal_write should maybe be modified to take extra unused parameter
	// it depends on if anyone ever needs to use it directly
	if (nbytes <= 0) {
		// returns -1 for nbytes < 0 and 0 for nbytes == 0
		return -(nbytes < 0);
	}
	return terminal_write(buf, nbytes);
}

/* switch_video_memory
 * DESCRIPTION: moves current vga text-mode screen data to one terminal's storage, and moves another one to the screen
 * INPUTS: termOld - the screen to be stored
 *		   termNew - the screen to be pulled out of memory and displayed
 * OUTPUTS: none
 * RETURN VAL: -1 on invalid arguments, 0 on success
 * NOTES: changes the screen's appearance
 */

int switch_video_memory(int termOld, int termNew)
{
	int i;
	
	//error checking
	if (termOld < 0 || termOld > MAX_NUM_TERMS)
		return -1;
	if (termNew < 0 || termNew > MAX_NUM_TERMS)
		return -1;
	
	//iterate over video memory	
	for(i = 0; i < VIDEOMEMSIZE; i++)
	{
		*(uint8_t *)(terms[termOld].video_mem_start+i) = *(uint8_t *)(video_mem_screen+i); //move old screen out of video display memory and into storage
		*(uint8_t *)(video_mem_screen+i) = *(uint8_t *)(terms[termNew].video_mem_start+i); //put new screen into video memory
	
	}
	return 0;

}
