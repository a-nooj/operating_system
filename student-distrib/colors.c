/*colors.c*/
/*implementation of the change_colors system call*/
#include "colors.h"



 
extern volatile int mouse_x;
extern volatile int mouse_y;
extern char old_attribute;

/*
 * change_colors
 *   DESCRIPTION: Sets the colors used for the current text mode screen
 *   INPUTS: buf - an 80x25 array of characters, which will be written in as the terminal's attribute bytes
 *   OUTPUTS: buf - old values of the attribute bytes will be written to this buffer. Used when restoring the screen's colors.
 *   RETURN VALUE: 0 on success, -1 on failure.
 *   SIDE EFFECTS: sets a signal handler.
 */
extern int32_t change_colors( unsigned char * buf)
{
	int i = 0;
	int caller_terminal;
	int size = NUM_COLS*NUM_ROWS;
	unsigned char temp;

	
	caller_terminal = current_terminal;
	
	//check for stupid user
	if(buf == 0)
		return -1;
	
	//disable curser during write to prevent curser multiplication bug
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = old_attribute;
	
	for(i = 0; i < size; i++)
	{
		//exchange buffer attribute with attribut in video memory
		temp = *(unsigned char *)(terms[caller_terminal].video_mem_start + 2*i + 1);
		*(unsigned char *)(terms[caller_terminal].video_mem_start + 2*i + 1) = buf[i];
		buf[i] = temp;
	}
	//reinable curser
	(*(uint8_t *)(terms[current_terminal].video_mem_start + ((terms[current_terminal].xmax*mouse_y + mouse_x) << 1) + 1)) = CURSER_ATTRIBUTE;

	return 0;
}
