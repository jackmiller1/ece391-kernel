#ifndef __INTERRUPT_TABLE_H
#define __INTERRUPT_TABLE_H

/**
 * Function to set up interrupts for the system.
  * Only needs to be called once upon boot.
  */
int init_interrupts(void);

#endif
