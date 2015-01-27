#include "kb.h"

//scancode to key mappings based on special key presses

uint8_t without_shift[NUM_KEYS] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
										'-', '=', '\0', '\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 
										'\0', '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 
										'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '};
										
uint8_t with_shift[NUM_KEYS] = {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', 
									 '_', '+', '\0', '\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 
									 '\0', '\0', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', '\0', '\\', 
									 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\0', '\0', '\0', ' '};
										
uint8_t with_capslock[NUM_KEYS] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
										'-', '=', '\0', '\0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 
										'\0', '\0', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', '\0', '\\', 
										'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '\0', '\0', '\0', ' '};


int char_num = 0;
int i;
int32_t keyboard_buffer[BUFFER_SIZE];
int read_flag, enter_flag, clear_flag, to_read = 0;

int32_t term_open (int32_t fd) {
	clear();

	set_screen_xy(0,0);
	update_cursor(0,0);
	
	read_flag=0;
	enter_flag=0;
	
	enable_irq(KEYBOARD_IRQ_NUM);

	return 0;
}

int32_t term_close (int32_t fd) {
	disable_irq(KEYBOARD_IRQ_NUM);
	return 0;
}

int32_t term_read (int32_t fd, void* buf, int32_t nbytes) {
	int num_bytes_read = 0;
	unsigned char * charbuf = (unsigned char *) buf;

	if (buf == NULL || nbytes < 0 || nbytes > 1024)
		return -1;
		
	//initialize keyboard buffer
	for (i = 0; i < BUFFER_SIZE; i++)
		keyboard_buffer[i] = '\0';
		
	read_flag = 1;
	enter_flag=0;
	
	to_read = nbytes;
	//keep waiting if enter not pressed 
	while (enter_flag == 0);

	//initialize charbuf
	for (i = 0; i < nbytes; i++)
		charbuf[i] = '\0';
	
	//copy data from keyboard_buffer to charbuf
	for (i = 0; i < nbytes; i++) {
		charbuf[i] = keyboard_buffer[i];
		num_bytes_read++;
	}
	
	read_flag=0;

	return num_bytes_read;
}

int32_t term_write (int32_t fd, const void* buf, int32_t nbytes) {
	int num_bytes_written = 0;
	unsigned char * charbuf = (unsigned char *) buf;

	if (charbuf == NULL || nbytes < 0 || nbytes > 128)
		return -1;

	for (i = 0; i < nbytes; i++) {
		putc(charbuf[i]);
		num_bytes_written++;
	}
	
	update_screen_loc(get_screen_x(), get_screen_y());
	
	char_num=0;
	
	return num_bytes_written;
}

void kb_handler() {
	unsigned char scancode = inb(IO_DATA_PORT);
	handle_scancode(scancode);
	send_eoi(KEYBOARD_IRQ_NUM);
}

void set_fn_flags(unsigned char scancode) {
	switch(scancode) {
		case CTRL_MAKE: 
			ctrl_pressed = 1;
			break;
		case LEFT_SHIFT_MAKE: 
			shift_pressed = 1;
			break;
		case RIGHT_SHIFT_MAKE: 
			shift_pressed = 1;
			break;
		case CAPSLOCK_MAKE: 
			capslock_pressed ^= 1;
			break;
		case CTRL_BREAK: 
			ctrl_pressed = 0;
			break;
		case LEFT_SHIFT_BREAK: 
			shift_pressed = 0;
			break;
		case RIGHT_SHIFT_BREAK: 
			shift_pressed = 0;
			break;
		case BACKSPACE_MAKE: 
			backspace_pressed = 1;
			break;
		case BACKSPACE_BREAK: 
			backspace_pressed = 0;
			break;
		case ENTER_MAKE: 
			enter_pressed = 1;
			break;
		case ENTER_BREAK: 
			enter_pressed = 0;
			break;
	}
}

void handle_scancode(unsigned char scancode) {
	uint8_t key;
	
	set_fn_flags(scancode);
	
	if(!read_flag) return;
	
	clear_flag = 0;
	
	if (ctrl_pressed && (scancode == SCANCODE_L)) {
		clear();
		
		set_screen_xy(0,0);
		update_cursor(0,0);

		for(i=0;i<char_num;i++)
			putc(keyboard_buffer[i]);
			
		update_cursor(get_screen_x(),get_screen_y());
		clear_flag = 1;
	}
	else {
		if (shift_pressed)
			key = with_shift[scancode];
		else {
			if (capslock_pressed)
				key = with_capslock[scancode];
			else
				key = without_shift[scancode];
		}
	}
	
	if (enter_pressed)
		key = '\n';
	
	if(backspace_pressed) {
		update_screen_loc(get_screen_x(),get_screen_y());
		keyboard_buffer[char_num]='\0';
		update_screen_loc(get_screen_x()-1,get_screen_y());
		printf(" ");
		update_screen_loc(get_screen_x()-1,get_screen_y());
	}
	
	move_to_buffer(scancode, key);
}

void move_to_buffer(unsigned char scancode, uint8_t key) {
	if((scancode >= SCANCODE_ONE && scancode <= SCANCODE_EQUALS) || 
	   (scancode >= SCANCODE_Q && scancode <= SCANCODE_RIGHT_SQ_BRACE) || 
	   (scancode >= SCANCODE_A && scancode <= SCANCODE_BACK_TICK) || 
	   (scancode >= SCANCODE_BACK_SLASH && scancode <= SCANCODE_FORWARD_SLASH) || 
		scancode == SCANCODE_SPACE || key == '\n') {
		//reset keyboard_buffer if enter is pressed
		if (key == '\n') {
			if(!enter_flag) { 
				enter_flag=1;
				
				if(!read_flag) {
					for (i = 0; i < BUFFER_SIZE; i++)
						keyboard_buffer[i] = '\0';
					char_num = 0;
					enter_flag=0;
				}
			}
		}
		
		if (char_num < to_read && clear_flag == 0){
			keyboard_buffer[char_num] = key;
			putc(key);
			char_num++;
		}
	}
}
