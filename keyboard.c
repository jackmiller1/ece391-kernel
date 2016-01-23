/*
*	keyboard.c - handles interrupts received by keyboard and allows 
*				 interfacing between keyboard and processor
*/
#include "keyboard.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"
#include "x86_desc.h"
#include "system_calls.h"
#include "terminal.h"

/*
Notes/References:

Linux driver for keyboard:
https://github.com/torvalds/linux/blob/master/drivers/input/keyboard/atkbd.c

"8042" PS/2 Controller:
http://wiki.osdev.org/%228042%22_PS/2_Controller
*/

// 0: neither shift or caps
// 1: shift enabled
// 2: caps enabled
// 3: both are enabled
static uint8_t key_mode = 0;
volatile uint8_t enter_flag = 0;

// Index of the last item in the keyboard buffer
volatile uint8_t key_buffer_idx = 0;

// whether the keyboard is enabled
volatile uint8_t keyboard_enabled = 1;

// Key buffer terminated by return key
volatile uint8_t *key_buffer;

// 0: ctrl is not pressed
// 1: ctrl is pressed
static uint8_t ctrl_state = UNPRESSED;

static uint8_t alt_state = UNPRESSED;

/* KEYBOARD SCANCODE */
static uint8_t scancode_map[KEY_MODES][KEY_COUNT] = {
	// no caps / no shift
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
	 'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'},
	// no caps / shift
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0', '\0', 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V', 
	 'B', 'N', 'M', '<', '>', '?', '\0', '*', '\0', ' ', '\0'},
	// caps / no shift
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0', '\0', 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V', 
	 'B', 'N', 'M', ',', '.', '/', '\0', '*', '\0', ' ', '\0'},
	// caps / shift
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0', '\0', 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"', '~', '\0', '\\', 'z', 'x', 'c', 'v', 
	 'b', 'n', 'm', '<', '>', '?', '\0', '*', '\0', ' ', '\0'}
};


/*
*	Function: init_keyboard()
*	Description: This function initializes the keyboard to the appropriate
*				 IRQ Line on the PIC (this being Line#1)
*	inputs:		nothing
*	outputs:	nothing
*	effects:	enables line 1 on the master PIC
*/
void 
init_keyboard(void) {
	enable_irq(KEYBOARD_IRQ_LINE);
}


/*
*	Function: init_keyboard_handler()
*	Description: This function reads from the appropriate port on the keyboard to 
*				receive the interrupts generated, parses this information, and 
*				displays the associated character on the screen.
*	inputs:	 nothing
*	outputs: nothing
*	effects: prints character to screen from scancode_map
*/
void 
keyboard_interrupt_handler() {
	cli();
	uint8_t c = 0;
	do {
		if (inb(KEYBOARD_DATA_PORT) != 0) {
			c = inb(KEYBOARD_DATA_PORT);
			if (c > 0) {
				break;
			}
		}
	} while(1);

	switch (c) {
		case LSHIFT_DOWN:
		case RSHIFT_DOWN:
			ENABLE_SHIFT();
			break;
		case LSHIFT_UP:
		case RSHIFT_UP:
			DISABLE_SHIFT();
			break;
		case CAPS_LOCK:
			TOGGLE_CAPS();
			break;
		case BACKSPACE:
			handle_backspace();
			break;
		case ENTER:
			handle_enter();
			break;
		case CTRL_DOWN:
			ctrl_state = PRESSED;
			break;
		case CTRL_UP:
			ctrl_state = UNPRESSED;
			break;
		case ALT_DOWN:
			alt_state = PRESSED;
			break;
		case ALT_UP:
			alt_state = UNPRESSED;
			break;
		case F1_KEY:
			if (alt_state == PRESSED) {
				send_eoi(KEYBOARD_IRQ_LINE);
				launch_term(TERMINAL_ONE);
			}
			break;
		case F2_KEY:
			if (alt_state == PRESSED) {
				send_eoi(KEYBOARD_IRQ_LINE);
				launch_term(TERMINAL_TWO);
			}
			break;
		case F3_KEY:
			if (alt_state == PRESSED) {
				send_eoi(KEYBOARD_IRQ_LINE);
				launch_term(TERMINAL_THREE);
			}
			break;
		default:
			handle_key_press(c);
			break;
	}
	
	send_eoi(KEYBOARD_IRQ_LINE);
	sti();
}

/*
*	Function: handle_key_press(uint8_t scancode)
*	Description: This function handles how to interpret characters pressed
*				the keyboard
*	inputs:	 keyboard scan code
*	outputs: nothing
*	effects: modifies the contents displayed on the screen
*/
void
handle_key_press(uint8_t scancode) {


	// Handle unknown scancodes
	if (scancode >= KEY_COUNT) {
		return;
	}

	uint8_t key = scancode_map[key_mode][scancode];

	// None character keys are handled in interrupt handler
	if (key == NULL_KEY) {
		return;
	}

	if (ctrl_state == PRESSED) {
		switch(key) {
			case 'l':
				clear();
				set_screen_pos(0,0);
				break;
			case 'c':
				return;
				break;
		}
	}

	else if ((key_buffer_idx < KEY_BUFFER_SIZE) && (keyboard_enabled == 1)) {
		append_to_key_buff(key);
		putc(key);
	}
}


/*
*	Function: append_to_key_buff(uint8_t key)
*	Description: This function adds a single key to the end of the key buffer
*	inputs:	 they key to append to the buffer
*	outputs: none
*	effects: increments key_buffer_idx and modifies content of key_buffer
*/
void
append_to_key_buff(uint8_t key) {
	if (key_buffer_idx < KEY_BUFFER_SIZE) {
		key_buffer[key_buffer_idx++] = key;
	}
}

/*
*	Function: clear_key_buffer()
*	Description: This function clears the key buffer
*	inputs:	 none
*	outputs: none
*	effects: sets key_buffer_idx to 0 and clears content of key_buffer
*/
void
clear_key_buffer() {
	uint8_t i;
	for (i = 0; i < KEY_BUFFER_SIZE; i++) {
		key_buffer[i] = NULL_KEY;
	}
	key_buffer_idx = 0;
}

/*
*	Function: handle_enter()
*	Description: handler for the Enter key
*	inputs:	 none
*	outputs: none
*	effects: sets enter flag
*/
void 
handle_enter() {
	get_pcb_ptr_process(terms[current_term_id].active_process_number)->term->enter_flag = 1;
	key_buffer[key_buffer_idx++] = '\n';
	enter();
}

/*
*	Function: handle_backspace()
*	Description: handler for the Backspace key
*	inputs:	 none
*	outputs: none
*	effects: modifies key buffer
*/
void 
handle_backspace() {
	if (key_buffer_idx > 0) {
		backspace();
		key_buffer_idx--;
		key_buffer[key_buffer_idx] = NULL_KEY;
	}
}
