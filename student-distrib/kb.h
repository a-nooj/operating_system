/* keyboard_handler.h - Defines for function to handle keyboard inputs
*/

#ifndef _KEYBOARD_HANDLER_H
#define _KEYBOARD_HANDLER_H

#include "types.h"
#include "lib.h"
#include "i8259.h"

#define IO_DATA_PORT 0x60
#define KEYBOARD_IRQ_NUM 1

#define BUFFER_SIZE 128

//scancodes for special keys
#define CTRL_MAKE 0x1D
#define LEFT_SHIFT_MAKE 0x2A
#define RIGHT_SHIFT_MAKE 0x36
#define CAPSLOCK_MAKE 0x3A
#define CTRL_BREAK 0x9D
#define LEFT_SHIFT_BREAK 0xAA
#define RIGHT_SHIFT_BREAK 0xB6
#define BACKSPACE_MAKE 0x0E
#define BACKSPACE_BREAK 0x8E
#define ENTER_MAKE 0x1C
#define ENTER_BREAK 0x9C

//secondary scancodes for error checking
#define SCANCODE_ONE 0x02
#define SCANCODE_EQUALS 0x0D
#define SCANCODE_Q 0x10
#define SCANCODE_RIGHT_SQ_BRACE 0x1B
#define SCANCODE_A 0x1E
#define SCANCODE_BACK_TICK 0x29
#define SCANCODE_BACK_SLASH 0x2B
#define SCANCODE_FORWARD_SLASH 0x35
#define SCANCODE_SPACE 0x39
#define SCANCODE_L 0x26

#define NUM_KEYS 58

extern int32_t keyboard_buffer[BUFFER_SIZE];

//flags for special keys
volatile uint32_t backspace_pressed, ctrl_pressed;
volatile uint32_t shift_pressed, capslock_pressed, enter_pressed;

void kb_handler();

//terminal system call functions
int32_t term_read (int32_t fd, void* buf, int32_t nbytes);
int32_t term_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t term_open (int32_t fd);
int32_t term_close (int32_t fd);

//secondary helper functions
void set_fn_flags(unsigned char scancode);
void handle_scancode(unsigned char scancode);
void move_to_buffer(unsigned char scancode, uint8_t key);

#endif
