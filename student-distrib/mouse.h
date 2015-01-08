/*mouse.h*/

#include "lib.h"
#include "x86_desc.h"
#include "debug.h" 
#include "i8259.h"
#include "terminal.h"
#include "process.h"

#define CURSER_ATTRIBUTE 0xF2
#define DEFAULT_MOUSE_SENSITIVITY 3 //LOWER means more sensitive

//global variables
volatile int ls_pending_flag;
volatile char executeable_name[32];
volatile int shortcut_pending_flag;
volatile int executable_name_length;


//local functions
void mouse_init(void);
void mouse_packet_handler(void);
