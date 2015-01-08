/*soundblaster.c*/
#include "soundblaster.h"


#define LONG_DELAY 9000000

#define NOTE_C 0
#define NOTE_D 2
#define NOTE_E 4
#define NOTE_F 5
#define NOTE_G 7
#define NOTE_A 9
#define NOTE_B 11

#define NOTE_ON 0x20
#define NOTE_OFF 0x00


//frequency_lookup: provides integers for frequencies to be sent to the soundcard based.
//note order is C, D, D#, E, F, F#, G, G#, A, A#, B
//from http://www.phy.mtu.edu/~suits/notefreqs.html
static unsigned int frequency_lookup[12] = {262,277,294,311,330,349,370,392,415,440,466,494};
extern volatile int soundcard_is_open;

/* test_sound
 * DESCRIPTION: debug function for testing soundcard functionality
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VAL: something?
 * NOTES: isn't actually used in the OS anywhere...
 */ 

int test_sound(void)
{
	int i = 0;
	unsigned char low_freq_byte = 0;
	unsigned char high_freq_byte = 0;
	unsigned char octave = 0;
	//outb(Data, Port)
	
	//reset all register values to 0
	for(i = 0; i < 0xF5; i++)
	{
		outb(i, MONO_ADDR_PORT);
		outb(0x00, MONO_DATA_PORT);
	}
	i = 0;

	//adLib card test: 
	//referenced from http://web.archive.org/web/20110720105924/http://www.shipbrook.com/jeff/sb.html
	while(1){
	
		//Set the modulator's multiple to 1
		outb(0x20, MONO_ADDR_PORT);
		outb(0x01, MONO_DATA_PORT);
		
		//Set the modulator's level to about 40 dB
		outb(0x40, MONO_ADDR_PORT);
		outb(0x10, MONO_DATA_PORT);
		
		//Modulator attack: quick; decay: long
		outb(0x60, MONO_ADDR_PORT);
		outb(0xF0, MONO_DATA_PORT);
		
		//Modulator sustain: medium; release: medium
		outb(0x80, MONO_ADDR_PORT);
		outb(0x77, MONO_DATA_PORT);
		
		low_freq_byte = (unsigned char)(frequency_lookup[i] & 0x00FF);
		
		//Set voice frequency's LSB (it'll be a D#)
		outb(0xA0, MONO_ADDR_PORT);
		outb(low_freq_byte, MONO_DATA_PORT);
		
		//Set the carrier's multiple to 1
		outb(0x23, MONO_ADDR_PORT);
		outb(0x01, MONO_DATA_PORT);
		
		//Set the carrier to maximum volume (about 47 dB)
		outb(0x43, MONO_ADDR_PORT);
		outb(0x00, MONO_DATA_PORT);
		
		//Carrier attack: quick; decay: long
		outb(0x63, MONO_ADDR_PORT);
		outb(0xF0, MONO_DATA_PORT);
		
		//Carrier sustain: medium; release: medium
		outb(0x83, MONO_ADDR_PORT);
		outb(0x77, MONO_DATA_PORT);
		
		
		high_freq_byte = (unsigned char)((frequency_lookup[i] & 0xFF00) >> 8);
		//Turn the voice on; set the octave and freq MSB
		outb(0xB0, MONO_ADDR_PORT);
		outb((NOTE_ON | (octave << 2) | high_freq_byte), MONO_DATA_PORT);
		
		delay(LONG_DELAY);
		delay(LONG_DELAY);
		
		//Turn the voice off
		outb(0xB0, MONO_ADDR_PORT);
		outb((NOTE_OFF | (octave << 2) | high_freq_byte), MONO_DATA_PORT);
		
		


		
		if(++i == 12)
		{
			i = 0;
			octave++;
			octave = octave % 8;
		}
	}





	return 0;
}

/* startup_beep
 * DESCRIPTION: plays a beep from the PC speaker to indicate the operating system is being booted
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: blocks while playing the sound. Restores all pertinant device registers to ensure PIT functions as normal
 */ 

void startup_beep()
{
	unsigned char byte;
	unsigned char port43byte;

	
	// Pulse the PC speaker to indicate OS is starting
	// see http://courses.engr.illinois.edu/ece390/books/labmanual/io-devices-speaker.html
	port43byte = inb(0x43); //save reg 0x43
	outb(182,0x43); //set up speaker on Ch.2 control byte
	outb(0x5F,0x42); // set up frequency (LSB)
	outb(0x00,0x42); // set up frequency (MSB)
	byte = inb(0x61); //set lowest two bits of port 61
	byte = byte | 0x03;
	outb(byte, 0x61);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	
	byte = inb(0x61); //clear lowest two bits of port 61
	byte = byte & ~(0x03);
	outb(byte, 0x61);
	
	//admire the ascii for a long time!
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	delay(LONG_DELAY);
	
	outb(port43byte,0x43); //restore 43, the control register
						   // ensures that the PIT generates interrupts for process switching


}


/* delay
 * DESCRIPTION: blocks for a specified period of time
 * INPUTS: cycles - the length of the pause, in for loop interations
 *
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: used to induce pauses between port writes for slower devices.
 */
void delay(long cycles)
{
	volatile long i;
	for(i = 0; i < cycles; i++);
}


/* play_melody
 * DESCRIPTION: plays a song from the AdLib Sound card. Will act as the "Write" function for the card.
 * INPUTS: buf - string containing instructions on how to play the song
 *		   nbytes - length of buf in bytes
 *
 * OUTPUTS: nothing
 * RETURN VAL: 0 on successful completion, -1 on input parse error
 * NOTES: input string must conform to the following standard:
 *  ->inital number, indicating the tempo
 *	->sequence of tones follow: 	
 *      ->tones dilenated by '|' character
 *      ->each tone consists of 5 characters
 *	    ->first character = letter A-F indicating note, or R for rest (silent)
 *      ->second character = #, b, or N to make note sharp, flat, or natural. No effect if tone is a rest.
 *      ->third character = note octave, number from 0 to 9. No effect if the tone is a rest.
 *      ->fourth character = note length, from 1 to 9. each step is twice as long as the next, eg a length of 5 is twice as long as a length of 4.
 *	    ->fifth character = letter a,b,c, or d, indicating what sound to play. 
 *	-> last character should be 'x' to indicate end of song
 *
 *  example (A major scale): 500|AN42a|BN42a|C#52a|DN52a|EN52a|F#52a|G#52a|AN53a|RN54ax
 *  only one note can be played at a time, currently.
 */ 
int play_melody(file_desc_t* fd, const void* buf, int32_t nbytes)
{

	int bufindex; //current character we're working with in the buffer
	char tempoascii[10] = {0,0,0,0,0,0,0,0,0,0};
	unsigned int tempo;
	int notelength = 0;
	int octave = 0;
	int exitflag = 0;
	char * buffer;
	char single_char_buffer[2] = {'\0', '\0'};
	unsigned char low_freq_byte = 0;
	unsigned char high_freq_byte = 0;
	int note = 0;
	int restflag = 0;
	
	buffer = (char *)buf;
	
	//check for a stupid user
	if(buffer == 0)
		return -1;
	
	//parse out and convert tempo
	for(bufindex = 0; buffer[bufindex] != '|'; bufindex ++)
	{
		if(bufindex >= 9)
			return -1;
		tempoascii[bufindex] = buffer[bufindex];
	}
	
	tempo = atoi(tempoascii);
	
	//incriment bufindex and check to see if we've hit the end of the buffer
	if( ++bufindex >= nbytes)
		return -1; //return error condition, since this should be the first tone
	//bufindex should now point to first note
	
	
	//play song:
	
	cli();
	exitflag = 0;
	while(exitflag == 0)
	{
		restflag = 0; //reset rest flag
	
		//parse note
		switch(buffer[bufindex])
		{
		
			case 'A':
				note = NOTE_A;
			break;
			
			case 'B':
				note = NOTE_B;
			break;
			
			case 'C':
				note = NOTE_C;
			break;
			
			case 'D':
				note = NOTE_D;
			break;
			
			case 'E':
				note = NOTE_E;
			break;
			
			case 'F':
				note = NOTE_F;
			break;
			
			case 'G':
				note = NOTE_G;
			break;
			
			case 'R':
				restflag = 1;
			break;
			
			default:
				sti();
				return -1;
		}
		
		//incriment bufindex and check to see if we've hit the end of the buffer
		if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
		
		//parse note modifier
		switch(buffer[bufindex])
		{
		
			case '#':
				note++; //increase note by one semitone
			break;
			
			case 'b':
				note--; //decrease note by one semitone
			break;
			
			case 'N': //do nothing ot the note, just a placeholder
			break;
			
			default:
				sti();
				return -1;
		}
		
		//incriment bufindex and check to see if we've hit the end of the buffer
		if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
		
		single_char_buffer[0] = buffer[bufindex];
		octave = atoi(single_char_buffer); //parse out octave
		if(note == -1) //adjust note roll-over to ajacent octave
		{
			note = 0;
			octave--;
		}
		if(note == 12)
		{
			note = 0;
			octave++;
		}
		if(octave > 8 || octave < 1) //ensure validity of octave
		{
			sti();
			return -1;
		}
		
		//incriment bufindex and check to see if we've hit the end of the buffer
		if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
		
		single_char_buffer[0] = buffer[bufindex];
		notelength = atoi(single_char_buffer); //parse out notelength
		if(notelength > 9 || notelength < 1) //ensure validity of notelength
		{
			sti();
			return -1;
		}
		
		//incriment bufindex and check to see if we've hit the end of the buffer
		if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
			
		//parse sound requested
		switch(buffer[bufindex])
		{
		
			case 'a':
				//Set the modulator's multiple to 1
				outb(0x20, MONO_ADDR_PORT);
				outb(0x01, MONO_DATA_PORT);

				//Set the modulator's level to about 40 dB
				outb(0x40, MONO_ADDR_PORT);
				outb(0x10, MONO_DATA_PORT);

				//Modulator attack: quick; decay: long
				outb(0x60, MONO_ADDR_PORT);
				outb(0xF0, MONO_DATA_PORT);

				//Modulator sustain: medium; release: medium
				outb(0x80, MONO_ADDR_PORT);
				outb(0x77, MONO_DATA_PORT);
				
				//disable feedback
				outb(0xC0, MONO_ADDR_PORT);
				outb(0x00, MONO_DATA_PORT);
				
				//waveform select
				outb(0xE0, MONO_ADDR_PORT);
				outb(0x00, MONO_DATA_PORT);
			
			break;
			
			case 'b':
				//Set the modulator's multiple to 1, enable amplititude modulation
				outb(0x20, MONO_ADDR_PORT);
				outb(0x81, MONO_DATA_PORT);
				
				//Set the modulator's level to about 10 dB
				outb(0x40, MONO_ADDR_PORT);
				outb(0x10, MONO_DATA_PORT);
				
				//Modulator attack: quick; decay: long
				outb(0x60, MONO_ADDR_PORT);
				outb(0xF0, MONO_DATA_PORT);
				
				//Modulator sustain: medium; release: medium
				outb(0x80, MONO_ADDR_PORT);
				outb(0x77, MONO_DATA_PORT);
				
				//disable feedback
				outb(0xC0, MONO_ADDR_PORT);
				outb(0x00, MONO_DATA_PORT);
				
				//waveform select
				outb(0xE0, MONO_ADDR_PORT);
				outb(0x01, MONO_DATA_PORT);

			break;
			
			case 'c':
				//Set the modulator's multiple to 1
				outb(0x20, MONO_ADDR_PORT);
				outb(0x01, MONO_DATA_PORT);
				
				//Set the modulator's level to about 10 dB
				outb(0x40, MONO_ADDR_PORT);
				outb(0x10, MONO_DATA_PORT);
				
				//Modulator attack: quick; decay: long
				outb(0x60, MONO_ADDR_PORT);
				outb(0xF0, MONO_DATA_PORT);
				
				//Modulator sustain: medium; release: medium
				outb(0x80, MONO_ADDR_PORT);
				outb(0x77, MONO_DATA_PORT);
				
				//enable feedback
				outb(0xC0, MONO_ADDR_PORT);
				outb(0x60, MONO_DATA_PORT);
				
				//waveform select
				outb(0xE0, MONO_ADDR_PORT);
				outb(0x02, MONO_DATA_PORT);

			break;
			
			case 'd':
				//Set the modulator's multiple to 1, with vibrato and amplititue modulation
				outb(0x20, MONO_ADDR_PORT);
				outb(0xC1, MONO_DATA_PORT);
				
				//Set the modulator's level to about 10 dB
				outb(0x40, MONO_ADDR_PORT);
				outb(0x10, MONO_DATA_PORT);
				
				//Modulator attack: quick; decay: long
				outb(0x60, MONO_ADDR_PORT);
				outb(0xF0, MONO_DATA_PORT);
				
				//Modulator sustain: medium; release: medium
				outb(0x80, MONO_ADDR_PORT);
				outb(0x77, MONO_DATA_PORT);
				
				//disable feedback
				outb(0xC0, MONO_ADDR_PORT);
				outb(0x00, MONO_DATA_PORT);
				
				//waveform select
				outb(0xE0, MONO_ADDR_PORT);
				outb(0x03, MONO_DATA_PORT);

			break;
			
			
			default:
				sti();
				return -1;
		}
		
		//incriment bufindex and check to see if we've hit the end of the buffer
		if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
		
		if(buffer[bufindex] == 'x') //detect end of song condition
			exitflag = 1;
		else if(buffer[bufindex] != '|') //check for error in tone input string
		{
			sti();
			return -1;
		}
		//incriment bufindex and check to see if we've hit the end of the buffer
		else if( ++bufindex >= nbytes)
		{
			sti();
			return -1; //return error condition, since this is in the middle of the tone
		}
	
		
		if(restflag != 1) //if this tone is not a rest...
		{
			low_freq_byte = (unsigned char)(frequency_lookup[note] & 0x00FF);
			high_freq_byte = (unsigned char)((frequency_lookup[note] & 0xFF00) >> 8);
		
			//Set voice frequency's LSB (it'll be a D#)
			outb(0xA0, MONO_ADDR_PORT);
			outb(low_freq_byte, MONO_DATA_PORT);
			
			//Turn the voice on; set the octave and freq MSB
			outb(0xB0, MONO_ADDR_PORT);
			outb((NOTE_ON | (octave << 2) | high_freq_byte), MONO_DATA_PORT);
		}
		
		sti();
		//arbitrary constant to account for the fact that delay needs a big number, and tempo is in the range from 1 to 9
		delay((tempo*5000) << notelength); //pause for the length of this note
		cli();
		
		if(restflag != 1) //if this tone is still not a rest
		{ 
			//Turn the voice off
			outb(0xB0, MONO_ADDR_PORT);
			outb((NOTE_OFF | (octave << 2) | high_freq_byte), MONO_DATA_PORT);
		}
		

	}

	
	sti();
	return 0; //we're done with the song!




}

/* atoi
 * DESCRIPTION: changes characters, 0 through 9, into their integer representation
 * INPUTS: string - null-terminated string of integers 
 * OUTPUTS: none
 * RETURN VAL: nothing
 * NOTES: assumes input is in the range representable by an integer (no overflow check). Any errors cause the function to return 0.
 */
int atoi(char * string)
{
	int i = 0;
	int j = 0;
	unsigned int digits_length = 0;
	int multiplier = 1;
	int digits[10] = {0,0,0,0,0,0,0,0,0,0};
	long tens_multiplier = 1;
	long running_sum;
	
	if(string == NULL) //check for null-input error
		return 0;
	
	digits_length = strlen(string);
	
	if(string[0] == '-') //see if a leading negative sign indicates a negative result requested
	{
		multiplier = -1;
		i = 1;
		digits_length--;
	}
	
	j = digits_length-1;
	
	while(string[i] != '\0' && i < 10) //iterate over all the characters, extracting out 
	{
		if(string[i] < '0' || string[i] > '9')//detect part of the string not being a number
			return 0;	
		digits[j--] = (int)string[i++] - (int)('0'); //put integer values into array in correct order (least-signifigant digit in digits[0])
	}
	
	for(j = 0, running_sum = 0; j < digits_length; j++) //calculate integer value from digits
	{
		running_sum += tens_multiplier * digits[j];
		tens_multiplier = tens_multiplier * 10;
	}

	return running_sum * multiplier;
}

/* AdLib_open
 * DESCRIPTION: initalizes the soundcard, locks it out so other processes can't use it (not virtualized
 * INPUTS: none
 * OUTPUTS: sets soundcard_is_open bit
 * RETURN VAL: 0 on success, -1 on failure
 * NOTES: none
 */
int32_t AdLib_open(void)
{
	int i;
	cli();
	//make sure only one user accesses the soundcard at a time
	if(soundcard_is_open != 0)
	{
		sti();
		return -1;
	}
	soundcard_is_open = 1;
	
	//initalize the soundcard:

	//reset all soundcard register values to 0
	for(i = 0; i < 0xF5; i++)
	{
		outb(i, MONO_ADDR_PORT);
		outb(0x00, MONO_DATA_PORT);
	}
	i = 0;
	
	//Allow for selection of different waveforms
	outb(0x01, MONO_ADDR_PORT);
	outb(0x20, MONO_DATA_PORT);
	
	//Set the modulator's multiple to 1
	outb(0x20, MONO_ADDR_PORT);
	outb(0x01, MONO_DATA_PORT);
	
	//Set the modulator's level to about 40 dB
	outb(0x40, MONO_ADDR_PORT);
	outb(0x10, MONO_DATA_PORT);
	
	//Modulator attack: quick; decay: long
	outb(0x60, MONO_ADDR_PORT);
	outb(0xF0, MONO_DATA_PORT);
	
	//Modulator sustain: medium; release: medium
	outb(0x80, MONO_ADDR_PORT);
	outb(0x77, MONO_DATA_PORT);
	
	//Set the carrier's multiple to 1
	outb(0x23, MONO_ADDR_PORT);
	outb(0x01, MONO_DATA_PORT);
	
	//Set the carrier to maximum volume (about 47 dB)
	outb(0x43, MONO_ADDR_PORT);
	outb(0x00, MONO_DATA_PORT);
	
	//Carrier attack: quick; decay: long
	outb(0x63, MONO_ADDR_PORT);
	outb(0xF0, MONO_DATA_PORT);
	
	//Carrier sustain: medium; release: medium
	outb(0x83, MONO_ADDR_PORT);
	outb(0x77, MONO_DATA_PORT);
	
	//end initalization
	sti();
	return 0;
}

/* AdLib_close
 * DESCRIPTION: closes out the soundcard, making it available for other processes
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VAL: 0 always
 * NOTES: none
 */
 int32_t AdLib_close(void) {
	soundcard_is_open = 0;
	return 0;
}
