/*soundblaster.h*/
/*header for all functions that work with the AdLib and pcSpeaker sound output devices*/


#include "x86_desc.h"
#include "lib.h"

#define MONO_ADDR_PORT 0x0388
#define MONO_DATA_PORT 0x0389

//global var
volatile int soundcard_is_open;

//functions
int test_sound();
void delay(long cycles);
void startup_beep(void);
int atoi(char * string);
int32_t play_melody(file_desc_t* fd, const void * buf, int32_t nbytes);
int32_t AdLib_open(void);
int32_t AdLib_close(void);

