/* terminal.h - Functions used by the terminal to display the screen and process read/write/open/close requests
 * vim:ts=4 noexpandtab
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H
 
#include "x86_desc.h"
#include "lib.h"
#include "debug.h" 
#include "i8259.h"
#include "types.h"
#include "keyboard.h"
#include "handlers.h"
#include "file_descriptor.h"
#include "process.h"
#include "signal.h"

#define KEY_DOWN 0x01
#define KEY_UP 0x00


//key codes for Scancode Set 1 for modifier keys
#define EXTENDED_KEYCODE 0xE0
#define RIGHT_CTRL_PRESS 0x1D
#define RIGHT_ALT_PRESS 0x38
#define RIGHT_SHIFT_PRESS 0x36
#define RIGHT_CTRL_RELEASE 0x9D
#define RIGHT_ALT_RELEASE 0xB8
#define RIGHT_SHIFT_RELEASE 0xB6
#define LEFT_CTRL_PRESS 0x1D
#define LEFT_ALT_PRESS 0x38
#define LEFT_SHIFT_PRESS 0x2A
#define LEFT_CTRL_RELEASE 0x9D
#define LEFT_ALT_RELEASE 0xB8
#define LEFT_SHIFT_RELEASE 0xAA
#define F1_PRESS 0x3B
#define F2_PRESS 0x3C
#define F3_PRESS 0x3D

#define ENTER_PRESSED 0x1C

#define BACKSPACE 0x0E

#define BLANK_CHAR 0x00
#define CURSER_CHAR 221

#define KEYBOARD_PORT 0x60
#define KEYBOARD_ENABLE 0xF4
#define KEYBOARD_IRQ 1

#define CHARATTRIB 0x07
#define CURSERATTRIB 0x04

#define TERM0ATTRIB 0x05
#define TERM1ATTRIB 0x06
#define TERM2ATTRIB 0x02
#define UNKTERMATTRIB 0x04
#define TERM_SWITCH_ATTRIB 0x82

#define NUM_COLS 80
#define NUM_ROWS 25

#define INPUT_BUF_SIZE 99999 //needs to be large enough to keep track of all characters written to terminal for entirety of computer operation...
#define READ_BUF_MAX 128 //max number of chararacters that can be returned from a read() call to the terminal

#define MAX_NUM_TERMS 3

#define TRUE 1
#define FALSE 0

/*global var saying what the current terminal is*/
extern int current_terminal;

/*function def's*/
extern int32_t terminal_init(void);
extern int32_t terminal_read(void * buf, int32_t nbytes);
extern int32_t terminal_write(const void * buf, int32_t nbytes);
extern int32_t terminal_write_kernel(const void * buf, int32_t nbytes, int term_num); //Allows terminal printing from the kernel
extern int32_t terminal_open(void);
extern int32_t terminal_close(void);
void terminal_kbd_handler(void);
extern int32_t current_terminal_process(int newPID);
int switch_video_memory(int termOld, int termNew);

// potentially temporary interfacing functions
int32_t stdin_read(file_desc_t* file_desc, void* buf, int32_t nbytes);
int32_t stdout_write(file_desc_t* file_desc, const void* buf, int32_t nbytes);

typedef struct {

	//modifier key statuses
	//1 if either of the alt keys are pressed, 0 otherwise. See pound-defines above
	volatile unsigned short ralt; //left modifiers
	volatile unsigned short rshift;
	volatile unsigned short rctrl;
	volatile unsigned short lalt; //left modifiers
	volatile unsigned short lshift;
	volatile unsigned short lctrl;
	
	int xpos; //current curser location on screen. 0,0 is top left.
	int ypos; //need to be signed in case the curser goes off the screen on the top or left
	
	unsigned int xmax; //maximum positions in x and y
	unsigned int ymax;
	
	char * video_mem_start; //memory location of video memory
	
	volatile unsigned char readbuf [READ_BUF_MAX];
	volatile int read_in_progress; //1 if the read function is waiting on keyboard input, 0 else.
								   //set by read, cleared by terminal_kbd_handler
	volatile int readbuf_index;
	int readbuf_size;
	
	volatile int last_x_written; //set by the write() system call - make sure the user can't backspace over
	volatile int last_y_written; //text written by the system

	volatile int process_on_this_term; //current PID runnning in this terminal

} terminal_data;

//data struct for keeping track of the three terminals
extern terminal_data terms[MAX_NUM_TERMS];

#endif
