/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#include "types.h"

#define VIDEO 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25
#define ROW_START 0
#define VIDEO_SIZE	(NUM_COLS*NUM_ROWS)
#define ATTRIB_TERM1 0xf
#define ATTRIB_TERM2 0x4
#define ATTRIB_TERM3 0x2
#define BLUESCREEN 0x16

/*Global Vars*/
extern volatile uint8_t keyboard_enabled;

/*Function Definitions -- SEE LIB.C DEFINITIONS FOR MORE DETAIL */
int get_screen_x(void);

int get_screen_y(void);

int32_t printf(int8_t *format, ...);

int32_t printf_terminal_running(int8_t *format, ...);

void putc(uint8_t c);

void putc_terminal_running(uint8_t c);

int32_t puts(int8_t *s);

int32_t puts_terminal_running(int8_t* s);

int8_t *itoa(uint32_t value, int8_t* buf, int32_t radix);

int8_t *strrev(int8_t* s);

uint32_t strlen(const int8_t* s);

void clear(void);

void set_screen_pos(uint32_t x, uint32_t y);

void set_screen_pos_term_exec(uint32_t x, uint32_t y);

void enter(void);

void enter_term_exec(void);

void backspace(void);

void scroll_up(void);

void scroll_up_term_exec();

void set_cursor_pos(void);

void turn_screen_blue(void);

void print_cr3(void);

void* memset(void* s, int32_t c, uint32_t n);

void* memset_word(void* s, int32_t c, uint32_t n);

void* memset_dword(void* s, int32_t c, uint32_t n);

void* memcpy(void* dest, const void* src, uint32_t n);

void* memmove(void* dest, const void* src, uint32_t n);

int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);

int8_t* strcpy(int8_t* dest, const int8_t*src);

int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);

void test_interrupts(void);


/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port)
{
	uint32_t val;
	asm volatile("xorl %0, %0\n \
			inb   (%w1), %b0" 
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return val;
} 

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port)
{
	uint32_t val;
	asm volatile("xorl %0, %0\n   \
			inw   (%w1), %w0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port)
{
	uint32_t val;
	asm volatile("inl   (%w1), %0"
			: "=a"(val)
			: "d"(port)
			: "memory" );
	return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
	asm volatile("outb  %b1, (%w0)"     \
			:                           \
			: "d" (port), "a" (data)    \
			: "memory", "cc" );         \
} while(0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
	asm volatile("outw  %w1, (%w0)"     \
			:                           \
			: "d" (port), "a" (data)    \
			: "memory", "cc" );         \
} while(0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
	asm volatile("outl  %l1, (%w0)"     \
			:                           \
			: "d" (port), "a" (data)    \
			: "memory", "cc" );         \
} while(0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
	asm volatile("cli"                  \
			:                       \
			:                       \
			: "memory", "cc"        \
			);                      \
} while(0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
	asm volatile("pushfl        \n      \
			popl %0         \n      \
			cli"                    \
			: "=r"(flags)           \
			:                       \
			: "memory", "cc"        \
			);                      \
} while(0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
	asm volatile("sti"                  \
			:                       \
			:                       \
			: "memory", "cc"        \
			);                      \
} while(0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
	asm volatile("pushl %0      \n      \
			popfl"                  \
			:                       \
			: "r"(flags)            \
			: "memory", "cc"        \
			);                      \
} while(0)


	
	
/*
* ASM Wrapper to show "blue screen" and print exception_name
*/	
#define EXCEPTION_THROWN(exception_name,msg)  	\
void exception_name() {							\
	keyboard_enabled = 0;						\
	clear();									\
	print_cr3();								\
	set_screen_pos(0,0);						\
	printf("%s\n",#msg); 						\
	turn_screen_blue();							\
	while(1);									\
}										\


#endif /* _LIB_H */
