/*
*	rtc.h - Function Header File for "rtc.c"
*/
#ifndef _RTC_H
#define _RTC_H

#include "types.h"

/* This defines both the index and data port on the RTC 
*	 Names are given according to both RTC Data Sheet
*    and names taken from OSDev
*/
#define RTC_PORT	0x70	//Index Register
#define CMOS_PORT	0x71	//Data Register


/* Registers on the RTC */
#define RTC_REGISTER_A	0x0A 
#define	RTC_REGISTER_B	0x0B 
#define	RTC_REGISTER_C	0x0C 
#define	RTC_REGISTER_D	0x0D 


/* PIC Interrupt Line */
#define RTC_IRQ_LINE 8


/* Initialize RTC */
void init_rtc(void);

/* Open the RTC */
int32_t rtc_open(const uint8_t* filename);

/* Read the RTC */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* Write to the RTC */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

/* Set Frequency on RTC */
void rtc_set_freq(int32_t freq);

/* Close the RTC */
int32_t rtc_close(int32_t fd);

/* Test Interrupts used for Checkpoint 1 */
void rtc_interrupt_handler(void);


#endif
