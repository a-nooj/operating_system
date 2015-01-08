/*keyboard.h - Includes all functions associated with the keyboard initalization and keyboard IRQ handling functions*/
#include "x86_desc.h"
#include "lib.h"
#include "debug.h" 
#include "i8259.h"

#define KEY_DOWN 0x01
#define KEY_UP 0x00

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define KEYBOARD_IRQ 1
/*local function definitions*/
void keyboard_init(void);
char key(unsigned char keycode);
char keyfull(unsigned char keycode, unsigned short shift);


#endif
