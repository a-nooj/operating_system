/*mouse.c*/
/*all functions associated with making the mouse work*/
/*reference: http://wiki.osdev.org/Mouse_Input and http://forum.osdev.org/viewtopic.php?t=10247 */

#include "mouse.h"

#define MOUSE_DATA_PORT 0x60
#define MOUSE_IRQ 12
#define CONT_CHIP_PORT 0x64
#define RESET_FINISHED 0xAA
#define RESET_COMMAND 0xFF
#define MOUSE_COMMAND 0xD4
#define DEFAULT_SETTINGS 0xF6
#define ENABLE_STREAMING 0xF4
#define ENABLE_MOUSE 0xA8
#define INTERRUPT_ENABLE_REGISTER 0x20
#define INTERRUPT_ENABLE_BIT 0x02
#define BITS_IN_INTEGER 32
#define BITS_IN_BYTE 8
#define X_SIGN_BIT_MASK 0x10
#define Y_SIGN_BIT_MASK 0x20
#define LEFT_BUTTON_BITMASK 0x01


//global file-scope variables and such...
extern volatile char executeable_name[32];
extern volatile int executable_name_length;
extern volatile int ls_pending_flag;
extern volatile int shortcut_pending_flag;
int volatile mouse_x;
int volatile mouse_y;
#define VIDEO 0xB8000
#define VIDEOMEMSIZE 4096 // just above 2 * NUM_ROWS * NUM_COLS, needs to be a multiple of page size
static char* video_mem_screen = (char *)VIDEO;
static char* video_mems[3] = {(char *)(VIDEO+VIDEOMEMSIZE*1), (char *)(VIDEO+VIDEOMEMSIZE*2), (char *)(VIDEO+VIDEOMEMSIZE*3)};
int left_button;
int prev_left_button;
extern PCB * ProcessPCBs[MAX_PROCESSES+1];
unsigned char old_attribute;
static int mouse_sensitivity;


/*
 * mouse_init
 *   DESCRIPTION: sends all bytes and sets up data structures required to run the mouse
 *   INPUTS: none
 *   OUTPUTS: some bytes...
 *   RETURN VALUE: none
 */
void mouse_init(void)
{
	unsigned char status;
	
	request_irq(&mouse_packet_handler, MOUSE_IRQ);
	outb(ENABLE_MOUSE, CONT_CHIP_PORT); //enable external mouse device
	
	//enable interrupts on controller chip
	outb(INTERRUPT_ENABLE_REGISTER, CONT_CHIP_PORT);
	status = inb(MOUSE_DATA_PORT);
	status = (status | INTERRUPT_ENABLE_BIT);
	outb(MOUSE_DATA_PORT, CONT_CHIP_PORT);
	outb(status, MOUSE_DATA_PORT);
	
	outb(MOUSE_COMMAND, CONT_CHIP_PORT); //say this next command is for the mouse
	outb(RESET_COMMAND, MOUSE_DATA_PORT); //send command to reset mouse
	while(inb(MOUSE_DATA_PORT) != RESET_FINISHED); //wait for reset to finish
	
	outb(MOUSE_COMMAND, CONT_CHIP_PORT); //say this next command is for the mouse
	outb(DEFAULT_SETTINGS, MOUSE_DATA_PORT); //send command to set default mouse settings
	inb(MOUSE_DATA_PORT); //read in acknowledgement byte
	
	outb(MOUSE_COMMAND, CONT_CHIP_PORT); //say this next command is for the mouse
	outb(ENABLE_STREAMING, MOUSE_DATA_PORT); //send command to enable packet streaming
	inb(MOUSE_DATA_PORT); //read in acknowledgement byte
	
	enable_irq(MOUSE_IRQ); //keyboard interrupt enabled

	mouse_x = 0;
	mouse_y = 0;
	old_attribute = CHARATTRIB;
	ls_pending_flag = 0;
	shortcut_pending_flag = 0;
	mouse_sensitivity = DEFAULT_MOUSE_SENSITIVITY;		 
	return;

}

/*
 * mouse_packet_handler
 *   DESCRIPTION: gets the incoming bytes from the mouse, and deals with them approprately
 *   INPUTS: none
 *   OUTPUTS: will probably update either the clicked variable, and/or the mouse x/y variables
 *   RETURN VALUE: none
 *	 NOTES: called from interrupt context, so usual warnings apply here.
 */
void mouse_packet_handler(void)
{

	int i;
	unsigned char byte0 = inb(MOUSE_DATA_PORT);
	unsigned char byte1 = inb(MOUSE_DATA_PORT);
	unsigned char byte2 = inb(MOUSE_DATA_PORT);
	int delta_x = 0;
	int delta_y = 0;
	int left_button = 0;
	
	
	//detect bad packets
	if( ((byte0 & 0x08) == 0x00) || ((byte0 & 0xC0) != 0x00) )
	{
		send_eoi(MOUSE_IRQ);
		return;
	}
	
	left_button = byte0 & LEFT_BUTTON_BITMASK;
	
	delta_x = byte1;
	if(byte0 & X_SIGN_BIT_MASK)
		delta_x = delta_x | 0xFFFFFF00; //sign extend
		
	delta_y = byte2;
	if(byte0 & Y_SIGN_BIT_MASK)
		delta_y = delta_y | 0xFFFFFF00; //sign extend
	

	
	//reset old attribtue byte
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute;

	//find new mouse position
	mouse_x = mouse_x + delta_x/mouse_sensitivity; //scale by factor of 2 to make mouse more controllable.
	mouse_y = mouse_y - delta_y/mouse_sensitivity; //minus because mouse inverts y axis wrt screen
	
	//make sure curser stays on screen
	if(mouse_x < 0)
		mouse_x = 0;
	if(mouse_x > (NUM_COLS - 1))
		mouse_x = NUM_COLS - 1;
		
	if(mouse_y < 0)
		mouse_y = 0;
	if(mouse_y > (NUM_ROWS - 1))
		mouse_y = NUM_ROWS - 1;
	
	//flip new atribute byte
	old_attribute = (*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1));
	*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;

	
	if(left_button && mouse_x == 79 && mouse_y == 1) //if we hit the terminal switch button,
	{
		(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute; //restore attribute
		terms[current_terminal].video_mem_start = video_mems[current_terminal]; //set current process to write to its own storage buffer
		switch_video_memory(current_terminal, (current_terminal + 1)%MAX_NUM_TERMS); //change out the screen data
		terms[(current_terminal + 1)%MAX_NUM_TERMS].video_mem_start = video_mem_screen; //change next process's video memory to point to the active region for the screen
		map_user_vga_page((uint32_t) terms[ProcessPCBs[current_process]->terminal_num].video_mem_start);
		current_terminal = (current_terminal + 1)%MAX_NUM_TERMS; //switch current terminal to next one
		old_attribute = (*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)); //set new curser
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;
		
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
		old_attribute = (*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)); //set new curser
		*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1) = CURSER_ATTRIBUTE;
	}
	
	if(left_button && mouse_x == 79 && mouse_y == 2) //if we clicked the Menu button,
	{
		ls_pending_flag = 1;	//execute the ls program to print out the possible things in the os.
	}
	
	if(left_button && mouse_x < 32) //if we clicked somewhere where ls could have printed out
	{
		//copy the current line up to the first null character into the buffer
		i = 0;
		while(i < 32 && ((*(uint8_t *)((terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + i) << 1)))) != 0x00))
		{
			executeable_name[i] = *(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + i) << 1));
			i++;
		}
		executable_name_length = i;
		//set that a shortcut is ready to be executed
		shortcut_pending_flag = 1;	
	}
	
	send_eoi(MOUSE_IRQ);
	return;


}
