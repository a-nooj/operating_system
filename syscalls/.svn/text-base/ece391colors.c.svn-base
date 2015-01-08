#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define NUM_COLS 80
#define NUM_ROWS 25

int main ()
{
	int i, j;
	uint8_t tempbuf[NUM_ROWS*NUM_COLS];
	uint8_t startbuf[NUM_ROWS*NUM_COLS];
	
	ece391_change_colors(startbuf); //store inital buffer
	for(j = 0; j < 4000; j++)
	{
		//cause a seizure!!!
		for(i = 0; i < NUM_ROWS*NUM_COLS; i++)
			tempbuf[i] = (uint8_t)(((i+j) & 0x0F)|(((i-j)<<4) & 0xC0)|(((i+j%13)<<2) & 0x30));
		ece391_change_colors(tempbuf);
	}

	ece391_change_colors(startbuf);


    return 0;
}