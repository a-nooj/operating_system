//this is a temporary debugging file to hold test interrupt handler functions
//NONE OF THESE FILES ARE USED IN THE OS

#include "test_int_handlers.h"
#include "keyboard.h" //for getting qwerty keymap
#define KEYBOARD_PORT 0x60
#define KEYBOARD_ENABLE 0xF4
#define KEYBOARD_IRQ 1
#define RTC_IRQ 8



//define location of video memory
#define VIDEO 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25
static char* video_mem = (char *)VIDEO;



//global vars
unsigned char testchar = 0x5B;

void test_kbd_handler(void)
{
	unsigned char LastKey;
	int screen_y = 5;
	int screen_x = 50;
	unsigned char attrib = 0x60;
	
	LastKey = inb(KEYBOARD_PORT);//read in the byte we got interrupted for
	//clear();
	
	if(key(LastKey) != 0x00)
	{
		printf("                            Pressed %c\n", key(LastKey)); //print out key pressed
		
		//for CP1, print out character to a fixed location too, so the TA's won't have to work too hard to see the keypress
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = key(LastKey);
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = attrib;	
	}
	printf("                                LastKey = 0x%x\n", LastKey);


}

void test_rtc_handler(void)
{
	int screen_y = 6;
	int screen_x = 50;
	unsigned char attrib = 0x60;

	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = testchar;
    *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = attrib;
	
	//swap displayed character
	testchar ^= 0x01;
	
	printf("RTC interrupt!\n");


}
