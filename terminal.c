
#include "terminal.h"
#include "lib.h"
#include "system_calls.h"
#include "types.h"
#include "i8259.h"
#include "paging.h"
#include "scheduling.h"


/* Global Variables - used to update terminal */
volatile uint8_t current_term_id;
term_t terms[TERM_COUNT];


/*
*   Function: init_terms()
*   Description: Initializes the terminals
*   inputs: none
*   outputs: none
*   effects: 
*/
void init_terms() {
	uint8_t i;
	uint32_t j;
	for (i = 0; i < TERM_COUNT; i++) {
		terms[i].id = i;
		terms[i].active_process_number = -1;
		terms[i].running = 0;
		terms[i].x_pos = 0;
		terms[i].y_pos = 0;
		terms[i].key_buffer_idx = 0;
		terms[i].enter_flag = 0;
		for (j = 0; j < KEY_BUFFER_SIZE; j++)
			terms[i].key_buffer[j] = '\0';

		// Change to different address for each term
		remapWithPageTableToPage(_100MB, _100MB+((i+1)*_4KB), i+1);
		terms[i].video_mem = (uint8_t *)_100MB+((i+1)*_4KB);
		// clear the memory
		for (j = 0; j < NUM_ROWS*NUM_COLS; j++) {
			*(uint8_t *)(terms[i].video_mem + (j << 1)) = ' ';
			//set color attributes for each terminal
        	if (i == 0)
        		*(uint8_t *)(terms[i].video_mem + (j << 1) + 1) = ATTRIB_TERM1;
	    	if (i == 1)
        		*(uint8_t *)(terms[i].video_mem + (j << 1) + 1) = ATTRIB_TERM2;
	   		if (i == 2)
        		*(uint8_t *)(terms[i].video_mem + (j << 1) + 1) = ATTRIB_TERM3;
		}
	}

	// start up the first terminal
	key_buffer = terms[0].key_buffer;
	restore_term_state(0);
	current_term_id = 0;
	execute((uint8_t*)"shell");
}

/*
*   Function: launch_term(uint8_t term_id)
*   Description: Launches a terminal, updating flags and variables
*   inputs: term_id -- terminal number of the terminal to be launched
*   outputs: returns 0 on success
*/
int32_t
launch_term(uint8_t term_id) {

	cli();
	if (term_id > TERM_COUNT-1)
		return -1;

	if (term_id == current_term_id) 
		return 0;

	/* Terminal is already running - simply restoring state (keyboard buff, vidmem) */
	if (terms[term_id].running == 1) {
		if (switch_terminals(current_term_id, term_id) == -1)
			return -1;
		key_buffer = terms[term_id].key_buffer;
		current_term_id = term_id;
        /* Remap video memory to 136 MB */
        uint8_t * screen_start;
        vidmap(&screen_start);
        if (terms[current_term_executing].id != current_term_id) {
            remapVideoWithPageTable((uint32_t)screen_start, (uint32_t)terms[current_term_executing].video_mem);
        }

		return 0;
	}
	
	/* Terminal is NOT running and we need to set up new term + shell */
	// Save state of current term
	save_term_state(current_term_id);

	// Launch new term
	current_term_id = term_id;
	pcb_t * old_pcb = get_pcb_ptr_process(terms[current_term_executing].active_process_number);
	key_buffer = terms[term_id].key_buffer;
	restore_term_state(term_id);
	
	
	
    /* Save the ebp/esp of the process we are switching away from. */
    asm volatile("			\n\
                 movl %%ebp, %%eax 	\n\
                 movl %%esp, %%ebx 	\n\
                 "
                 :"=a"(old_pcb->ebp), "=b"(old_pcb->esp)
	);
	sti();
	execute((uint8_t*)"shell");
	return 0;
}

/*
*   Function: save_term_state(uint8_t term_id)
*   Description: saves the state of a terminal
*   inputs: term_id -- terminal number of the terminal to save the state of
*   outputs: returns 0 on success
*/
int32_t
save_term_state(uint8_t term_id) {
    /* Since the terminal we are switching away from is no longer being viewed, we need to map virtual vidmem to the buffer */
    //remapVideoWithPageTable(_136MB, (uint32_t)terms[term_id].video_mem);
    
	terms[term_id].key_buffer_idx = key_buffer_idx;

	terms[term_id].x_pos = get_screen_x();
	terms[term_id].y_pos = get_screen_y();

	memcpy((uint8_t *)terms[term_id].video_mem, (uint8_t *)VIDEO, 2*NUM_ROWS*NUM_COLS);

	return 0;
}

/*
*   Function: restore_term_state(uint8_t term_id)
*   Description: restores the state of a terminal
*   inputs: term_id -- terminal number of the terminal to restore the state of
*   outputs: returns 0 on success
*/
int32_t
restore_term_state(uint8_t term_id) {
	key_buffer_idx = terms[term_id].key_buffer_idx;

	set_screen_pos(terms[term_id].x_pos, terms[term_id].y_pos);
	memcpy((uint8_t *)VIDEO, (uint8_t *)terms[term_id].video_mem, 2*NUM_ROWS*NUM_COLS);

	return 0;
}

/*
*   Function: switch_terminals(uint8_t old_term_id, uint8_t new_term_id) {

*   Description: switches the current terminal
*   inputs: old_term_id -- terminal number of the terminal to switch away from
*			new_term_id -- terminal number of the terminal to switch to
*   outputs: returns 0 on success, -1 on failure
*/
int32_t 
switch_terminals(uint8_t old_term_id, uint8_t new_term_id) {
	if (save_term_state(old_term_id) == -1)
		return -1;

	if (restore_term_state(new_term_id) == -1)
		return -1;

	return 0;
}

/*
*	Function: terminal_open(const uint8_t* filename)
*	Description: Opens a terminal
*	inputs:	 nothing
*	outputs: nothing
*	effects: none
*/
int32_t 
terminal_open(const uint8_t* filename) {
	return 0;
}

/*
*	Function: terminal_close(int32_t fd)
*	Description: Closes a terminal
*	inputs:	 nothing
*	outputs: nothing
*	effects: none
*/
int32_t 
terminal_close(int32_t fd) {
	return 0;
}

/*
*	Function: keyboard_read(int8_t* buf, int32_t nbytes)
*	Description: This function reads data from one line that will has been terminated
*				by pressing Enter or until the buffer is full
*	inputs:	 pointer to a buffer and the size of the buffer
*	outputs: the number of bytes written to the buffer
*	effects: clears the current key buffer
*/
int32_t
terminal_read(int32_t fd, void* buf, int32_t nbytes) {

	uint8_t i;
	while (get_pcb_ptr()->term->enter_flag == 0);
	get_pcb_ptr()->term->enter_flag = 0;
	
	int8_t * buffer = (int8_t *)buf;

	for (i = 0; (i < nbytes-1) && (i < KEY_BUFFER_SIZE); i++) {
		buffer[i] = key_buffer[i];
	}

	clear_key_buffer();
	return i;
}

/*
*	Function: terminal_write(int8_t* buf, int32_t nbytes)
*	Description: Writes a string to the terminal
*	inputs:	 a pointer to a buffer and the number of btyes to write
*	outputs: the number of bytes displayed
*	effects: writes to screen
*/
int32_t 
terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
	int32_t temp;
	cli();
	if (current_term_id == current_term_executing)
		temp = printf((int8_t *)buf);
	else
		temp = printf_terminal_running((int8_t *)buf);
	sti();
	return temp;
}
