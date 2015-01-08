/*keyboard.c - Includes all functions associated with the keyboard initalization and keyboard driver*/
#include "keyboard.h"
#define KEYBOARD_PORT 0x60
#define KEYBOARD_ENABLE 0xF4
#define KEYMAP_SIZE 256

static char QWERTY_keymap [KEYMAP_SIZE]; //maps scancode set 1 keypress values to ascii values
static char full_keymap [KEYMAP_SIZE][2];  //maps scancode set 1 keypress values to ascii values for both upper and lower case


/* keyboard_ACK
 * DESCRIPTION: Performs all actions required to acknowledge a keyboard transmission
 * INPUTS: none
 *
 * OUTPUTS: none
 * RETURN VAL: none
 * NOTES: apparently, we dont' need a function to do this...skeleton is here if we ever need to use it....
 */
int keyboard_ACK(void) {

		return 0;
}

/* key
 * DESCRIPTION: takes a char representing keyboard input, and maps it to an ascii value associated with a keypress
 * INPUTS: keycode - input from keyboard in format from scancode set 1
 *
 * OUTPUTS: none
 * RETURN VAL: ascii value of keycode, if it was a key press
 * NOTES: none
 */
char key(unsigned char keycode) {
	return QWERTY_keymap[keycode];
}

/* keyfull
 * DESCRIPTION: takes a char representing keyboard input, and maps it to an ascii value associated with a keypress along with modifiers
 * INPUTS: keycode - input from keyboard in format from scancode set 1
 *         shift - determines if upper or lowercase characters should be used
 *
 * OUTPUTS: none
 * RETURN VAL: ascii value of keycode, if it was a key press
 * NOTES: none
 */
char keyfull(unsigned char keycode, unsigned short shift) {
	if(shift == KEY_DOWN) //case - lowercase values
		return full_keymap[keycode][1];
	else //case - uppercase values
		return full_keymap[keycode][0];
}


/* set_QWERTY_keymap
 * DESCRIPTION: sets up the keymap with proper values
 * INPUTS: none
 *
 * OUTPUTS: assigns proper values to proper indices within the QWERTY_keymap global variable
 * RETURN VAL: none
 * NOTES: should be called once before the key() function is called. Depends on global array QWERTY_keymap
 */
void set_QWERTY_keymap (void) {

	int i = 0;

	for(i = 0; i < 256; i++)
		QWERTY_keymap[i] = 0x00; //clear out entries

	QWERTY_keymap[2] = '1'; //set entries of keys we know we can get - only supports single chars (no shift/ctl/alt/function keys)
	QWERTY_keymap[3] = '2';
	QWERTY_keymap[4] = '3';
	QWERTY_keymap[5] = '4';
	QWERTY_keymap[6] = '5';
	QWERTY_keymap[7] = '6';
	QWERTY_keymap[8] = '7';
	QWERTY_keymap[9] = '8';
	QWERTY_keymap[10] = '9';
	QWERTY_keymap[11] = '0';

	QWERTY_keymap[16] = 'Q';
	QWERTY_keymap[17] = 'W';
	QWERTY_keymap[18] = 'E';
	QWERTY_keymap[19] = 'R';
	QWERTY_keymap[20] = 'T';
	QWERTY_keymap[21] = 'Y';
	QWERTY_keymap[22] = 'U';
	QWERTY_keymap[23] = 'I';
	QWERTY_keymap[24] = 'O';
	QWERTY_keymap[25] = 'P';

	QWERTY_keymap[30] = 'A';
	QWERTY_keymap[31] = 'S';
	QWERTY_keymap[32] = 'D';
	QWERTY_keymap[33] = 'F';
	QWERTY_keymap[34] = 'G';
	QWERTY_keymap[35] = 'H';
	QWERTY_keymap[36] = 'J';
	QWERTY_keymap[37] = 'K';
	QWERTY_keymap[38] = 'L';
	QWERTY_keymap[44] = 'Z';
	QWERTY_keymap[45] = 'X';
	QWERTY_keymap[46] = 'C';
	QWERTY_keymap[47] = 'V';
	QWERTY_keymap[48] = 'B';
	QWERTY_keymap[49] = 'N';
	QWERTY_keymap[50] = 'M';
	QWERTY_keymap[51] = ',';
	QWERTY_keymap[52] = '.';
	QWERTY_keymap[215] = '@';
	QWERTY_keymap[57] = ' ';
	QWERTY_keymap[53] = '/';
	QWERTY_keymap[0x29] = '`';
	QWERTY_keymap[0x39] = ' ';
	QWERTY_keymap[0x4A] = '-';
	QWERTY_keymap[0x55] = '=';
	QWERTY_keymap[0x1A] = '[';
	QWERTY_keymap[0x1B] = ']';
	QWERTY_keymap[0x2B] = 0x5C; //for " \ "
	QWERTY_keymap[0x27] = ';';
	QWERTY_keymap[0x28] = 0x27; //for single quote " ' "
	QWERTY_keymap[0x35] = '/';
	QWERTY_keymap[0x0C] = '-';
	QWERTY_keymap[0x0D] = '=';

}

/* set_full_keymap
 * DESCRIPTION: sets up the keymap with proper values for both upper and lowercase
 * INPUTS: none
 *
 * OUTPUTS: assigns proper values to proper indices within the QWERTY_keymap global variable
 * RETURN VAL: none
 * NOTES: should be called once before the key() function is called. Depends on global array QWERTY_keymap
 */
void set_full_keymap (void) {

	int i = 0;

	for(i = 0; i < 256; i++)
	{
		full_keymap[i][0] = 0x00; //clear out entries
		full_keymap[i][1] = 0x00;
	}

	//lowercase
	full_keymap[2][0] = '1'; //set entries of keys we know we can get - only supports single chars (no shift/ctl/alt/function keys)
	full_keymap[3][0] = '2';
	full_keymap[4][0] = '3';
	full_keymap[5][0] = '4';
	full_keymap[6][0] = '5';
	full_keymap[7][0] = '6';
	full_keymap[8][0] = '7';
	full_keymap[9][0] = '8';
	full_keymap[10][0] = '9';
	full_keymap[11][0] = '0';

	full_keymap[16][0] = 'q';
	full_keymap[17][0] = 'w';
	full_keymap[18][0] = 'e';
	full_keymap[19][0] = 'r';
	full_keymap[20][0] = 't';
	full_keymap[21][0] = 'y';
	full_keymap[22][0] = 'u';
	full_keymap[23][0] = 'i';
	full_keymap[24][0] = 'o';
	full_keymap[25][0] = 'p';

	full_keymap[30][0] = 'a';
	full_keymap[31][0] = 's';
	full_keymap[32][0] = 'd';
	full_keymap[33][0] = 'f';
	full_keymap[34][0] = 'g';
	full_keymap[35][0] = 'h';
	full_keymap[36][0] = 'j';
	full_keymap[37][0] = 'k';
	full_keymap[38][0] = 'l';
	full_keymap[44][0] = 'z';
	full_keymap[45][0] = 'x';
	full_keymap[46][0] = 'c';
	full_keymap[47][0] = 'v';
	full_keymap[48][0] = 'b';
	full_keymap[49][0] = 'n';
	full_keymap[50][0] = 'm';
	full_keymap[51][0] = ',';
	full_keymap[52][0] = '.';
	full_keymap[215][0] = 0x00;
	full_keymap[57][0] = ' ';
	full_keymap[53][0] = '/';
	full_keymap[0x29][0] = '`';
	full_keymap[0x39][0] = ' ';
	full_keymap[0x4A][0] = '-';
	full_keymap[0x55][0] = '=';
	full_keymap[0x1A][0] = '[';
	full_keymap[0x1B][0] = ']';
	full_keymap[0x2B][0] = 0x5C; //for " \ "
	full_keymap[0x27][0] = ';';
	full_keymap[0x28][0] = 0x27; //for single quote " ' "
	full_keymap[0x35][0] = '/';
	full_keymap[0x0C][0] = '-';
	full_keymap[0x0D][0] = '=';
	
	
	//uppercase
	full_keymap[2][1] = '!'; //set entries of keys we know we can get - only supports single chars (no shift/ctl/alt/function keys)
	full_keymap[3][1] = '@';
	full_keymap[4][1] = '#';
	full_keymap[5][1] = '$';
	full_keymap[6][1] = '%';
	full_keymap[7][1] = '^';
	full_keymap[8][1] = '&';
	full_keymap[9][1] = '*';
	full_keymap[10][1] = '(';
	full_keymap[11][1] = ')';

	full_keymap[16][1] = 'Q';
	full_keymap[17][1] = 'W';
	full_keymap[18][1] = 'E';
	full_keymap[19][1] = 'R';
	full_keymap[20][1] = 'T';
	full_keymap[21][1] = 'Y';
	full_keymap[22][1] = 'U';
	full_keymap[23][1] = 'I';
	full_keymap[24][1] = 'O';
	full_keymap[25][1] = 'P';

	full_keymap[30][1] = 'A';
	full_keymap[31][1] = 'S';
	full_keymap[32][1] = 'D';
	full_keymap[33][1] = 'F';
	full_keymap[34][1] = 'G';
	full_keymap[35][1] = 'H';
	full_keymap[36][1] = 'J';
	full_keymap[37][1] = 'K';
	full_keymap[38][1] = 'L';
	full_keymap[44][1] = 'Z';
	full_keymap[45][1] = 'X';
	full_keymap[46][1] = 'C';
	full_keymap[47][1] = 'V';
	full_keymap[48][1] = 'B';
	full_keymap[49][1] = 'N';
	full_keymap[50][1] = 'M';
	full_keymap[51][1] = '<';
	full_keymap[52][1] = '>';
	full_keymap[215][1] = 0x00;
	full_keymap[57][1] = ' ';
	full_keymap[53][1] = '?';
	full_keymap[0x29][1] = '~';
	full_keymap[0x39][1] = ' ';
	full_keymap[0x4A][1] = '_';
	full_keymap[0x55][1] = '+';
	full_keymap[0x1A][1] = '{';
	full_keymap[0x1B][1] = '}';
	full_keymap[0x2B][1] = '|';
	full_keymap[0x27][1] = ':';
	full_keymap[0x28][1] = 0x22; //for double quote " 
	full_keymap[0x35][1] = '?';
	full_keymap[0x0C][1] = '_';
	full_keymap[0x0D][1] = '+';

}


/* keyboard_init
 * DESCRIPTION: Performs all actions required to initalize keyboard
 * INPUTS: 
 *
 * OUTPUTS: 
 * RETURN VAL: 
 * NOTES: 
 */
void keyboard_init(void)
{

	enable_irq(KEYBOARD_IRQ); //keyboard interrupt enabled
	set_QWERTY_keymap();
	set_full_keymap();

	
	return;
}


