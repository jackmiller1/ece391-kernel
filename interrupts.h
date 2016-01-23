/*
*	interrupts.h - Function Header File to be used with "interrupts.S"
*/

#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H

/* Keyboard interrupt asm wrapper */
extern void keyboard_handler();

/* Clock interrupt asm wrapper */
extern void rtc_handler();

/* PIT interrupt asm wrapper */
extern void pit_handler();

/* System Call asm wrapper */
extern void system_call_handler();

#endif /* INTERRUPT_HANDLER_H */

