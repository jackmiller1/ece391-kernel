/*
*   rtc.c - functions called to interact with the RTC (Real Time Clock)
*/

#include "scheduling.h"
#include "i8259.h"
#include "lib.h"
#include "types.h"
#include "system_calls.h"
#include "paging.h"
#include "x86_desc.h"
#include "terminal.h"

/*Global Variables to keep track of:*/
/* Stores the current terminal that is executing the current process */
volatile uint8_t current_term_executing = 0;
uint8_t next_scheduled_term = 0;

/*
*   Function: init_PIT()
*   Description: This initializes the PIT to allow interrupts.
*   inputs: none
*   outputs: none
*   effects: effects IRQ Line 0 to allow interrupts from the PIT
*   background:
*       http://wiki.osdev.org/Programmable_Interval_Timer
*       http://www.osdever.net/bkerndev/Docs/pit.htm
*/
void 
init_PIT(void) {
    
    /*Scheduler set to interrupt at 20 second intervals*/
    outb(PIT_SQUARE_WAVE_MODE_3, PIT_COMMAND_REG);
    outb(_20HZ & FREQ_MASK, PIT_CHANNEL_0);
    outb(_20HZ >> _EIGHT, PIT_CHANNEL_0);
    
    /*Enable IRQ Line 0 for PIT Interrupts*/
    enable_irq(PIT_IRQ_LINE);
    return;
}

/*
*   Function: PIT_intterupt_and_schedule()
*   Description: This is called whenever a PIT interrupt is received, and calls for a context switch
*   inputs: none
*   outputs: none
*   effects: 
*/
void
PIT_interrupt_and_schedule() {
    
    /*Last line - send EOI to PIT */
    send_eoi(PIT_IRQ_LINE); 
    
    cli();
    if (terms[1].running == 1 || terms[2].running == 1) {
            doContextSwitch(get_next_scheduled_process());
    }
    sti();
    return;
}

/*
 *   Function: doContextSwitch(int processNumber);
 *   Description: Performs a context switch from the current process to another process
 *   inputs: process_number -- the number of the process to switch to
 *   outputs: none
 */
void doContextSwitch(int process_number) {
    /* Map a new page in virtual address 8MB to physical address 8 */
    remap(_128MB, _8MB + process_number * _4MB);

    /* Remap video memory to 136 MB */
    uint8_t * screen_start;
    vidmap(&screen_start);


    /* Get the PCB that we are changing FROM */
    //pcb_t * old_pcb = get_pcb_ptr();
    pcb_t * old_pcb = get_pcb_ptr_process(terms[current_term_executing].active_process_number);
    current_term_executing = next_scheduled_term;
    /* Get the PCB that we are switching INTO */
    pcb_t * next_pcb = get_pcb_ptr_process(process_number);
    

    /* Get the terminal that we are switching INTO */
    term_t * terminal = next_pcb->term;

    /* If terminal is not being displayed, map virtual video mem to buffer */
    if (terminal->id != current_term_id) {
        remapVideoWithPageTable((uint32_t)screen_start, (uint32_t)terminal->video_mem);
    }

    /* Save SS0 and ESP0 in TSS for context switching */
    tss.ss0 = KERNEL_DS;
    //I THINK THIS SHOULD BE old_pcb->process_number because we are using the TSS to restore back??*/
    tss.esp0 = _8MB - _8KB * (process_number) - 4;

     /* FOR DEBUGGING */
    //printf("Current Process Number: %d, Switching Into: %d\n", old_pcb->process_number, next_pcb->process_number);
    //printf("-----------------------------------------\n");
    /* Perform context switch, swap ESP/EBP with that of the registers */
    asm volatile(
                 "movl %%esp, %%eax;"
                 "movl %%ebp, %%ebx;"
                 :"=a"(old_pcb->esp), "=b"(old_pcb->ebp)    /* outputs */
                 :                                          /* no input */
                 );
                 
    asm volatile(
                 "movl %%eax, %%esp;"
                 "movl %%ebx, %%ebp;"
                 :                                          /* no outputs */
                 :"a"(next_pcb->esp), "b"(next_pcb->ebp)    /* input */
                 );
    return;
}

/*
 *   Function: get_next_scheduled_process()
 *   Description: calculates the process number of the next process to be executed
 *   inputs: none
 *   outputs: returns the process number of the next process to be executed
 *   effects: changes next_scheduled_term 
 */
int 
get_next_scheduled_process(){
    /*Loop from 0 to 2 depending on the available processes/active processes at the time */
    int i;
	
    next_scheduled_term = current_term_executing;

    for (i = 0; i < TERM_COUNT; i++)
    {
        next_scheduled_term = (next_scheduled_term + 1) % 3;
        if (terms[next_scheduled_term].running == 1)
            break;
    }

    /* Return the proper "next process number" */
    return (int)terms[next_scheduled_term].active_process_number;
    
}

