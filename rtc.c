/*
*   rtc.c - functions called to interact with the RTC (Real Time Clock)
*/

#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "types.h"
#include "system_calls.h"
#include "scheduling.h"
#include "terminal.h"


/* To keep track of state of interrupt */
volatile int rtc_interrupt_occurred [3] = { 0, 0, 0 };

/*
*   Function: init_rtc()
*   Description: This function initializes the appropriate ports on the RTC,
*				 initializes control register A and B, and sets frequency = 2 Hz.
*   inputs: none
*   outputs: none
*   effects: enables IRQ Line on PIC, sends appropriate values to RTC (according to DataSheet)
*/
void 
init_rtc(void){
	/* Select Control Register A, and set 0x80 to disable NMI */
 	outb(RTC_REGISTER_A, RTC_PORT); 
	
	/* Read current value of Register B */
 	unsigned char a_old = inb(CMOS_PORT); 
	
	/* Set the index again ( a read will reset the index to Register D) */
 	outb(RTC_REGISTER_B, RTC_PORT);
	
	/* Writ ethe previous value OR'd with 0x40. Turns on bit 6 of Register B */
    outb(a_old | 0x40, CMOS_PORT);
	
	/* Enable appropriate IRQ Line on PIC (Line #8) */
 	enable_irq(RTC_IRQ_LINE);
 }


/*
*	Function: rtc_interrupt_handler()
*	Description: This function is only implemented for Checkpoint 1
*				 and upon every RTC interrupt it calls test_interrupts()
*	input:	none
*	output: none
*	effects: (eventually) updates video memory and sends end of interrupt to RTC IRQ Line
*/
void rtc_interrupt_handler(void){
	/* Send EOI to IRQ Line */
	send_eoi(RTC_IRQ_LINE);
	int i;
	cli();
	for (i = 0; i < TERM_COUNT; i++)
	{
		rtc_interrupt_occurred[i] = 1; //change interrupt variable to show that it is occuring
	}
	outb(0x0C, RTC_PORT); 	//select register C
	inb(CMOS_PORT); 		//throw away contents
	sti();
}


/*
*	Function: rtc_open()
*	Description: This will be our main funciton to open the RTC
*	input: pointer to filename
*	output: returns 0 always
*	effects: Opens the rtc and initializes frequency to 2 HZ
*/
int32_t 
rtc_open(const uint8_t * filename){

	/* Set RTC Frequency to 2 Hz */
    rtc_set_freq(2);

    /* Always return 0 */
    return 0;
}
 

 /*
 *	Function: rtc_read()
 *	Description: This will read the contents within the RTC only after an interrupt has occured
 *	input: file descriptor, buffer to read into, and number of bytes
 *	output: returns 0 upon success
 *	effects: Reads the RTC
 */
int32_t 
rtc_read(int32_t fd, void* buf, int32_t nbytes){

	/* Disable interrupts and spin until interrupt received */
	while (!rtc_interrupt_occurred[current_term_executing]){
		/* SPIN */
	}
	
	/* Reset interrupt-occured */
	rtc_interrupt_occurred[current_term_executing] = 0;
	/* Returns the number of bytes read always */
	return 0;
 }


 /*
 *	Function: rtc_write()
 *	Description: 
 *	input: file descriptor, pointer to buffer being modified, number of bytes writing
 *	output: returns 0 always
 *	effects:
 */
int32_t 
rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    /* Local variables. */
	int32_t freq;

	/* Boundary check - ONLY 4 Bytes */	
	if (4 != nbytes || (int32_t)buf == NULL) 
		return -1;  /* Fail - always need to write 4 bytes) */
	else 
		freq = *((int32_t*)buf);

	/* Set the RTC Frequency to our variable freq */
	rtc_set_freq(freq);
	
	/* Return the number of bytes wrote always */
	return nbytes;   
}


/*
*   Function: rtc_set_frequency
*   Description: This function sets the frequency on the RTC to be user defined
*   input:	int freq = frequency desired
*   output: none
*   effects: sets RTC Frequency
*/
void
rtc_set_freq(int32_t freq){
    /* Local variables. */
    char rate;

    /* Save old value of Reg A*/
    outb(RTC_REGISTER_A, RTC_PORT);
    unsigned char a_old = inb(CMOS_PORT);

    /* Values defined in RTC Datasheet (pg.19) */
	if (freq == 8192 || freq == 4096 || freq == 2048) return;
    if (freq == 1024) rate = 0x06;
    if (freq == 512) rate = 0x07;
    if (freq == 256) rate = 0x08;
    if (freq == 128) rate = 0x09;
    if (freq == 64) rate = 0x0A;
    if (freq == 32) rate = 0x0B;
    if (freq == 16) rate = 0x0C;
    if (freq == 8) rate = 0x0D;
    if (freq == 4) rate = 0x0E;
    if (freq == 2) rate = 0x0F;

    /* set A[3:0] (rate) to rate */
    outb(RTC_REGISTER_A, RTC_PORT);
    outb((0xF0 & a_old) | rate, CMOS_PORT);
}


 /*
 *	Function: rtc_close()
 *	Description: This is the main function to close the RTC.
 *	input: pointer to file descriptor being closed
 *	output: returns 0 always
 *	effects: closes the RTC
 */
int32_t 
rtc_close(int32_t fd){
	
	 /* Reset RTC Frequency to 2 Hz */
    rtc_set_freq(2);

    /* Always return 0 */
    return 0;
 }
