/*
*	keyboard.h - Function Header File to be used with "keyboard.c"
*/
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

/* PIC Interrupt Line and IDT Vector Number */
#define KEYBOARD_IRQ_LINE	1
#define KEYBOARD_DATA_PORT	0x60
#define KEY_BUFFER_SIZE		127

/* Magic Numbers */
#define KEY_COUNT			60
#define KEY_MODES			4
#define BACKSPACE	0x0E
#define TAB			0x0F
#define CAPS_LOCK	0x3A
#define ENTER		0x1C
#define LSHIFT_DOWN	0x2A
#define LSHIFT_UP	0xAA
#define RSHIFT_DOWN	0x36
#define RSHIFT_UP	0xB6
#define CTRL_DOWN	0x1D
#define ALT_DOWN	0x38
#define CTRL_UP		0x9D
#define ALT_UP		0xB8
#define F1_KEY		0x3B
#define F2_KEY		0x3C
#define F3_KEY		0x3D
#define TERMINAL_ONE 0
#define TERMINAL_TWO 1
#define TERMINAL_THREE 2

#define UNPRESSED	0
#define PRESSED 	1

#define ENABLE_CAPS()			(key_mode |= 1 << 1)
#define DISABLE_CAPS()			(key_mode &= ~(1 << 1))
#define TOGGLE_CAPS()			(key_mode ^= 1 << 1)
#define ENABLE_SHIFT()			(key_mode |= 1)
#define DISABLE_SHIFT()			(key_mode &= ~(1))
#define APPEND_TO_KEY_BUFF(k)	key_buffer[key_buffer_idx++] = (k)
#define NULL_KEY			'\0'

extern volatile uint8_t key_buffer_idx;
extern volatile uint8_t enter_flag;
extern volatile uint8_t *key_buffer;

/* Initialize Keyboard Function */
extern void init_keyboard(void);

/* Keyboard Interrupt Handler Function */
extern void keyboard_interrupt_handler(void);

/* Clears the key board buffer */
extern void clear_key_buffer(void);

/* Adds key to key buffer */
void append_to_key_buff(uint8_t key);

/* Called when a character key is pressed*/
void handle_key_press(uint8_t scancode);

/*  Clears the key buffer */
void clear_key_buffer();

/* Called when enter key is pressed */
void handle_enter();

/* Called when backspace key is pressed */
void handle_backspace();



#endif /* _KEYBOARD_H */
