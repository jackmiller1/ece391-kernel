/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Ports that each PIC sits on */
#define MASTER_8259_PORT2 (MASTER_8259_PORT + 1)
#define SLAVE_8259_PORT2  (SLAVE_8259_PORT + 1)

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7 */
uint8_t slave_mask = 0xFF; /* IRQs 8-15 */

/*	Function: i8259_init(void)
*	Description: Initialize the 8259 PIC
*	input: none
*	output: 0 upon success
*	effects: sends ICWs (initialization control words) to PIC ports to initialize the PIC
*/
void 
i8259_init(void)
{
	// possible TODO? save masks at the start and restore them at the end? 
	
	// CLI is called in boot.S and STI is called at the end of entry
	//cli();
	
	//DO WE NEED TO FLUSH PIC BEFORE INITIALIZATION?? 
	// Sends ICW1 to the first PIC port for master and slave 
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW1, SLAVE_8259_PORT);

	// Sends ICW2 to the first PIC port for master and slave 
	// Note: different ICW2 signals for master and slave
	outb(ICW2_MASTER, MASTER_8259_PORT2);
	outb(ICW2_SLAVE, SLAVE_8259_PORT2);

	// Sends ICW3 to the first PIC port for master and slave 
	// Note: different ICW3 signals for master and slave
	outb(ICW3_MASTER, MASTER_8259_PORT2);
	outb(ICW3_SLAVE, SLAVE_8259_PORT2);

	// Sends ICW4 to the first PIC port for master and slave 
	outb(ICW4, MASTER_8259_PORT2);
	outb(ICW4, SLAVE_8259_PORT2);

	//Enabling the Slave IRQ Line (being on line #2)
	enable_irq(SLAVE_IRQ_LINE);
}

/*	Function: enable_irq(uint32_t irq_num)
*	Description: Enable (unmask) the specified IRQ
*	input: irq_num -- the number of the IRQ line to operate on
*	output: none
*	effects: unmasks the IRQ line specified by irq_num by simply 
* 			 writing an updated mask to the data port
*/
void
enable_irq(uint32_t irq_num)
{
	// Masking and unmasking of interrupts on an 8259A outside of the 
	//interrupt sequence requires only a single write to the second port and 
	//the byte written to this port specifies which interrupts should be masked
	
	/* Ret if irq_num is invalid */ 
 	if ((irq_num > 15) || (irq_num < 0)) { 
 		return; 
 	} 
	
 	/* initial mask = 11111110 - mask out everything but first line */ 
 	uint8_t mask = 0xFE; 
 
	/* MASTER BOUNDS = 0 -> 7*/
 	if ((irq_num >= 0) && (irq_num <= 7)) { 
 		int b; 
 		for (b = 0; b < irq_num; b++) { 
 			mask = (mask << 1) + 1; 
 		}
		/* send out to master line */
 		master_mask = master_mask & mask; 
 		outb(master_mask, MASTER_8259_PORT2); 
 		return; 
 	} 
 
 	/* SLAVE BOUNDS = 8 -> 15 */ 
 	if ((irq_num >= 8) && (irq_num <= 15)) { 
 		irq_num -= 8;
 		int b; 
 		for (b = 0; b < irq_num; b++) { 
 			mask = (mask << 1) + 1; 
 		} 
		/* send out to slave line */
 		slave_mask = slave_mask & mask; 
 		outb(slave_mask, SLAVE_8259_PORT2); 
 		return; 
 	} 
}

/*	Function: disable_irq(uint32_t irq_num)
*	Description: Disable (mask) the specified IRQ
*	input: irq_num -- the number of the IRQ line to operate on
*	output: none
*	effects: masks the IRQ line specified by irq_num by simply 
* 			 writing an updated mask to the data port
*/
void
disable_irq(uint32_t irq_num)
{
	/* Ret if irq_num is invalid */ 
 	if ((irq_num > 15) || (irq_num < 0)) { 
 		return; 
 	} 
	
 	/* initial mask = 11111110 */ 
 	uint8_t mask = 0x01; 
 
	/* MASTER BOUNDS = 0 -> 7 */
 	if ((irq_num >= 0) && (irq_num <= 7)) { 
 		int b; 
 		for (b = 0; b < irq_num; b++) { 
 			mask = (mask << 1); 
 		} 
		/*send out to master line */
 		master_mask = master_mask | mask; 
 		outb(master_mask, MASTER_8259_PORT2); 
 	} 
 
	/* SLAVE BOUNDS = 8 -> 15 */
 	if ((irq_num >= 8) && (irq_num <= 15)) { 
 		irq_num -= 8; 
 		int b; 
 		for (b = 0; b < irq_num; b++) { 
 			mask = (mask << 1); 
 		} 
		/*send out to slave line */
 		slave_mask = slave_mask | mask; 
 		outb(slave_mask, SLAVE_8259_PORT2); 
 	} 
}

/*	Function: send_eoi(uint32_t irq_num)
*	Description: Send end-of-interrupt signal for the specified IRQ
*	input: irq_num -- the number of the IRQ line to operate on
*	output: none
*	effects: ORs the EOI (end of interrupt) byte with the irq line number and
*			 sends it out to the PIC 
*/
void
send_eoi(uint32_t irq_num)
{
	/* MASTER BOUNDS = 0 -> 7 */
	if ((irq_num >= 0) && (irq_num <= 7)) { 
 		outb( EOI | irq_num, MASTER_8259_PORT2-1); 
 	} 
 
 	/* SLAVE BOUNDS = 8 -> 15*/
 	if ((irq_num >= 8) && (irq_num <= 15)) { 
 		outb( EOI | (irq_num - 8), SLAVE_8259_PORT2-1); 
 		outb( EOI + 2, MASTER_8259_PORT2-1); 
 	}  	
}

