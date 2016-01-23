#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "keyboard.h"

#define TERM_COUNT  3

/**TERMINAL STRUCT **/
typedef struct {
    // terminal id (ie. 0, 1, 2)
    uint8_t id;
	
	//active process number
	int8_t active_process_number;

    // whether terminal has a process running
    uint8_t running;

    // cursor position
    uint32_t x_pos;
    uint32_t y_pos;

    // key buffer for each terminal
    volatile uint8_t key_buffer[KEY_BUFFER_SIZE+1];
    volatile uint8_t key_buffer_idx;

    volatile uint8_t enter_flag;

    //ptr to video memory for terminal
    uint8_t *video_mem;
} term_t;

/* Global Variables */
extern volatile uint8_t current_term_id;
extern term_t terms[TERM_COUNT];

/*Function Definitions */
void init_terms(void);
int32_t launch_term(uint8_t term_id);
int32_t save_term_state(uint8_t term_id);
int32_t restore_term_state(uint8_t term_id);
int32_t switch_terminals(uint8_t old_term_id, uint8_t new_term_id);

/*Terminal System Calls */
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

#endif /* _TERMINAL_H */
