/*
*   system_calls.c - used to handle all system calls at int = 0x80
*/

#include "x86_desc.h"
#include "paging.h"
#include "types.h"
#include "lib.h"
#include "fileSystemModule.h"
#include "system_calls.h"
#include "terminal.h"
#include "scheduling.h"
#include "rtc.h"



/*******************
* Global Variables *
********************/
/* Process ID Array to start a new process - only can have 6 at a time */
uint8_t process_id_array [MAX_PROCESSES] = { 0,0,0,0,0,0 };


/* Initialize distinct fops tables for later use */
fops_table std_in_fops = {terminal_read, failure_function, terminal_open, terminal_close};
fops_table std_out_fops = {failure_function, terminal_write, terminal_open, terminal_close};
fops_table rtc_fops = {rtc_read, rtc_write, rtc_open, rtc_close};
fops_table dir_fops = {dir_read, dir_write, dir_open, dir_close};
fops_table file_fops = {file_read, file_write, file_open, file_close};
fops_table no_fops = {failure_function, failure_function, failure_function, failure_function};


/* 
*	Function halt()
*	Description: terminates a process, returning the specified value to its parent process
*	input: 	status -- the value to return to its parent process
*	output: returns status
*	effect: terminates the current process
*/
int32_t 
halt(uint8_t status){   
	/* Local Variables */ 
	int i;
	
	cli();

    /* Get current and parent PCB */
    pcb_t* current_pcb = get_pcb_ptr_process(terms[current_term_executing].active_process_number);
    pcb_t* parent_pcb = get_pcb_ptr_process(current_pcb->parent_process_number);

	/* Free up a spot in process_id_array */
    process_id_array[(uint8_t)current_pcb->process_number] = 0;

	
    /* set all present flags in PCB to "Not In Use" */
 	for (i = 0; i < MAX_FILES; i++)
 	{
 		if(current_pcb->fds[i].flags == IN_USE){
 			close(i);
 		}
		current_pcb->fds[i].fops_table_ptr = no_fops;
 		current_pcb->fds[i].flags = NOT_IN_USE;
 	}
	
	/* Update the terms array active process number to be the parent_pcb (process number that we are restoring to) */
	terms[current_term_executing].active_process_number = parent_pcb->process_number;
	
	/* Debugging */
	//printf("Halting Process Number: %d and Parent Process Number: %d \nNow restoring back to Process Number: %d, with Parent Process Number: %d\n", current_pcb->process_number, current_pcb->parent_process_number, current_pcb->parent_process_number, get_pcb_ptr_process(current_pcb->parent_process_number)->parent_process_number);

	/* Check if we are trying to halt the last process in a terminal */
	if (current_pcb->process_number == current_pcb->parent_process_number )
	{
		/* If we are trying to halt it, then we are going to execute another shell*/
		terms[current_term_executing].running = 0;
		execute((uint8_t *)"shell");
	}

    /* Restore Page Mapping */
    remap(_128MB, _8MB + parent_pcb->process_number * _4MB);
    
    /** set esp0 in tss */
	tss.esp0 = current_pcb->parent_ksp;
	
	sti();
    /* Return from iret */
    asm volatile(
				 ""
                 "mov %0, %%eax;"
                 "mov %1, %%esp;"
                 "mov %2, %%ebp;"
                 "jmp RETURN_FROM_IRET;"
                 :                      /* no outputs */
                 :"r"((uint32_t)status), "r"(current_pcb->parent_ksp), "r"(current_pcb->parent_kbp)   /* inputs */
                 :"%eax"                 /* clobbered registers */
                 );
    /* Should never reach this point due to iret *
   	 * ----------------------------------------- */
	return 0;
}


/* 
*	Function execute()
*	Description: This function executes the specified program, in the sequence as follows:
*		1. Parse Command
*		2. EXE Check
*		3. Reorganize Virtual Memory
*		5. File Loader
*		6. Process Control Block
*		7. Context/TSS Switch		
*	input: pointer to command you wish to execute (for example: "shell")
*	output: an integer returning the status of the function:
			-1  :
			256 :
*	effect:
*/
int32_t 
execute(const uint8_t* command){

	/* Disable interrupts?*/
	cli();

	/**** STACK VARIABLES ****/
	int i;
	int8_t parsed_command[MAX_COMMAND_SIZE], argument[MAX_BUFFER_SIZE];
	uint8_t buffer[READ_BUFFER_SIZE], command_end, command_start;
	int32_t new_process_number;
	uint32_t entry_point;

	/******************************************************
	 * FIRST: PARSE COMMAND & ARGUMENTS (ARGS IN CHKPT 4) *
     ******************************************************/
	 /* PARSE PROGRAM NAME */

     /*Find the start, handle leading spaces. Find the end, handling trailing zeroes  */
	command_end = command_start = 0;
	while (command[command_start] == ' ')
		command_start++;

	command_end = command_start;

	while (command[command_end] != ' ' && command[command_end] != ASCII_NL && command[command_end] != '\0')
		command_end++;
	
	for (i = command_start; i < command_end; i++)
		parsed_command[i - command_start] = (int8_t)command[i];
	parsed_command[command_end] = '\0';
	
	/* PARSE ARGUMENT */
	command_start = command_end + 1;
	command_end = command_start;
	while (command[command_end] != ' ' && command[command_end] != ASCII_NL && command[command_end] != '\0')
		command_end++;
	
	for (i = command_start; i < command_end; i++)
		argument[i - command_start] = (int8_t)command[i];
	
	argument[command_end-command_start] = '\0';
	
	/* PARSE IF EXIT */
	if (strncmp("exit", parsed_command, READ_BUFFER_SIZE) == 0)
	{
		asm volatile(
            "pushl	$0;"
            "pushl	$0;"
            "pushl	%%eax;"
            "call halt;"
			:
			);	
	}
	else if (strncmp("term_num", parsed_command, READ_BUFFER_SIZE) == 0)
	{
		printf("TERM %d\n", current_term_id);
	}
	
	/***********************
	 * SECOND: EXE CHECK *
     ***********************/
	dentry_t test_dentry;
	if (0 != read_dentry_by_name((uint8_t*)parsed_command, &test_dentry))
        return -1;
	
    /*check first 4 bytes for ELF */
    read_data(test_dentry.inodeNumber, 0, buffer, READ_BUFFER_SIZE);
    if ((buffer[0] != ASCII_DEL) || (buffer[1] != ASCII_E) || 
    	(buffer[2] != ASCII_L) || (buffer[3] != ASCII_F))
        return -1;
		
	
	 
    /* Obtain the entry point from bytes 24 -> 27 in exe file */
    read_data(test_dentry.inodeNumber, ENTRY_POINT_START, buffer, READ_BUFFER_SIZE);
    entry_point = *((uint32_t*)buffer);
    
	/* Get new process number */
	new_process_number = get_available_process_number();
	/* If we have no room for process, return -1 */
    if (new_process_number == -1)
    	return -1;
	/* Initializing the pcb ptr based on the process number*/
 	pcb_t * process_control_block = get_pcb_ptr_process(new_process_number);
	/* Saving the current ESP and EBP into the PCB struct */
	asm volatile("			\n\
				movl %%ebp, %%eax 	\n\
				movl %%esp, %%ebx 	\n\
			"
			:"=a"(process_control_block->parent_kbp), "=b"(process_control_block->parent_ksp));

	

	/***********************
	 * THIRD: SETUP PAGING *
     ***********************/

    /* Map a new page in virtual address 0x8000000 to physical address 0x800000 */
	remap(_128MB, _8MB + new_process_number * _4MB);


	/***********************
	 * FOURTH: FILE LOADER *
     ***********************/

	/* copy entire file to 0x08048000 in virtual memory*/
    read_data(test_dentry.inodeNumber, 0, (uint8_t*)LOAD_ADDRESS, LARGENUMBER);


	/***************************************
	 * FIFTH: SET UP PROCESS CONTROL BLOCK *
     ***************************************/
 	// get pcb of the parent
	pcb_t * parent_PCB;

 	/* Set up current process number = WILL NEED TO CHANGE FOR CHECKPOINT 4 */
 	process_control_block->process_number = new_process_number;

	if (terms[current_term_id].running == 0)//check if program is first one in terminal
	{
		current_term_executing = current_term_id;

		//set parent equal to self and mark terminal as running
		process_control_block->parent_process_number = process_control_block->process_number;
		terms[current_term_id].running = 1;
	}
	else 
	{
		/* Set parent process number normally */
		parent_PCB = get_pcb_ptr_process(terms[current_term_id].active_process_number);
		process_control_block->parent_process_number = parent_PCB->process_number;
	}

	/* Debugging */
	//printf("executing process number %d and parent number %d \n", process_control_block->process_number, process_control_block->parent_process_number);

	/* Store command in argument buffer in the PCB */
	strcpy(process_control_block->argbuf, argument);
	
	
	
 	/* Initializing each file descriptor to be default */
 	for (i = 0; i < MAX_FILES; i++)
 	{
 		/* Set to no operation for default */
		process_control_block->fds[i].fops_table_ptr = no_fops;
 		/* Set to 0xffff because our implementation uses inode number instead of inode ptr */
 		process_control_block->fds[i].inode = -1; 
 		/* Initialize file position to 0 */
		process_control_block->fds[i].file_position = FILE_START;
		/* Initialize present flag to "Not In Use" */
 		process_control_block->fds[i].flags = NOT_IN_USE;
 	}
	
	

	/**********************************************************
	 * SIXTH: INITIALIZE FDS[0] & FDS[1] TO BE STDIN & STDOUT *
     **********************************************************/
	process_control_block->fds[0].fops_table_ptr = std_in_fops;
	process_control_block->fds[1].fops_table_ptr = std_out_fops;
	process_control_block->fds[0].flags = IN_USE; 
	process_control_block->fds[1].flags = IN_USE;

	/****************************
	 * UPDATE TERMINAL DETAILS 	*
	 ****************************/
	/* Update the term number to be the current terminal we want to execute */
	process_control_block->term = &terms[current_term_id];
	/* Set active process number on our current terminal */
	terms[current_term_id].active_process_number = process_control_block->process_number;

	/************************
	 * LAST: CONTEXT SWITCH *
     ************************/

    /* Save SS0 and ESP0 in tss for context switching */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB - _8KB * (new_process_number) - 4;


    /*Re-enable Interrupts?*/
    sti();


    /* Pushing "artificial iret" onto stack */
    asm volatile(
                 "cli;"
                 "mov $0x2B, %%ax;"
                 "mov %%ax, %%ds;"
                 "movl $0x83FFFFC, %%eax;"
                 "pushl $0x2B;"
                 "pushl %%eax;"
                 "pushfl;"
                 "popl %%edx;"
                 "orl $0x200, %%edx;"
                 "pushl %%edx;"
                 "pushl $0x23;"
                 "pushl %0;"
                 "iret;"
                 "RETURN_FROM_IRET:;"
                 "LEAVE;"
                 "RET;"
                 :	/* no outputs */
                 :"r"(entry_point)	/* input */
                 :"%edx","%eax"	/* clobbered register */
                 );

    /* Should never reach this point due to iret *
   	 * ----------------------------------------- */
    return 0;
}

/* 
*	Function read()
*	Description: reads a file into buffer
*	input: file descriptor idx, buffer, and number of bytes
*	output: -1 if error, or return value from file read call
*	effect:
*/
int32_t 
read(int32_t fd, void* buf, int32_t nbytes){

    /* Get current PCB Pointer */
	pcb_t *pcb = get_pcb_ptr();
	/* Bounds Check 0 -> 7 */
	if (fd < 0 || fd > MAX_FD)
		return -1;
	/* Sanity checks */
	if (buf == NULL)
		return -1;
	/* check if file is unopened */
	if (pcb->fds[fd].flags == NOT_IN_USE) 
		return -1;

	/* Make the appropriate "read" system call */
	//int32_t temp = pcb->fds[fd].fops_table_ptr.read(fd, (char*)buf, nbytes);
	//pcb->fds[fd].file_position += temp;
	//return temp;
	return pcb->fds[fd].fops_table_ptr.read(fd, (char*)buf, nbytes);
}


/* 
*	Function write()
*	Description: calls appropriate write function for a file
*	input: file descriptor idx, buf to write, and the number of bytes
*	output: -1 if error, or return value from write call
*	effect:
*/
int32_t 
write(int32_t fd, const void* buf, int32_t nbytes){
	/* Get current PCB Pointer */
	pcb_t *pcb = get_pcb_ptr_process(terms[current_term_executing].active_process_number);
	/* Bounds Check 0 -> 7 */
	if (fd < 0 || fd > MAX_FD)
		return -1;
	/* Sanity checks */
	if (buf == NULL)
		return -1;
	/* check if file is unopened */
	if (pcb->fds[fd].flags == NOT_IN_USE) 
		return -1;

	/* Make the appropriate "write" system call */
	return pcb->fds[fd].fops_table_ptr.write(fd, (char *)buf, nbytes);
}

/* 
*	Function open()
*	Description: opens a file into the current pcb
*	input: filename -- name of the file to open
*	output: -1 if error, or index in file descriptor array
*	effect: opens file
*/
int32_t 
open(const uint8_t* filename){
	uint16_t fd_idx;
	pcb_t *pcb = get_pcb_ptr();
	dentry_t file_dir_entry;
	if (read_dentry_by_name(filename, &file_dir_entry) == -1)
	{
		return -1;
	}

	for (fd_idx = MIN_FD; fd_idx <= MAX_FD; fd_idx++) {
		if (pcb->fds[fd_idx].flags == NOT_IN_USE) {
			pcb->fds[fd_idx].flags = IN_USE;
			pcb->fds[fd_idx].file_position = FILE_START;
			break;
		}
		else if (fd_idx == MAX_FD ) {
			/* FDS is full */
			return -1;
		}
	}

	// set inode and fops_table_ptr
	switch (file_dir_entry.fileType) 
	{
		case RTC_TYPE:
			if (0 != rtc_open(filename))
				return -1;
			pcb->fds[fd_idx].inode = NULL;
			pcb->fds[fd_idx].fops_table_ptr = rtc_fops;
			break;
		case DIR_TYPE:
			if (0 != dir_open(filename))
				return -1;
			pcb->fds[fd_idx].inode = NULL;
			pcb->fds[fd_idx].fops_table_ptr = dir_fops;
			break;
		case FILE_TYPE:
			if (0 != file_open(filename))
				return -1;
			pcb->fds[fd_idx].inode = file_dir_entry.inodeNumber;
			pcb->fds[fd_idx].fops_table_ptr = file_fops;
			break;
	}
	return fd_idx;
}


/* 
*	Function close()
*	Description: closes a file
*	input: fd -- file descriptor index
*	output: 0 if successful, -1 if error
*	effect: closes file
*/
int32_t 
close (int32_t fd){
	/* Bounds Check 2 -> 7 */
	if (fd < MIN_FD || fd > MAX_FD)
		return -1;

	/* Get the current PCB pointer */
	pcb_t *pcb = get_pcb_ptr();
	
	/* File is not in use so it cant be closed.. */
	if (pcb->fds[fd].flags == NOT_IN_USE)
		return -1;

	/* Set file to unused */
	pcb->fds[fd].flags = 0;
	
	/*Close the file */
	if (0 != pcb->fds[fd].fops_table_ptr.close(fd))
		return -1;
	
	return 0;
}

/* 
*	Function getargs()
*	Description: reads the program’s command line arguments into a user-level buffer
*	input: 	buf -- pointer to buffer to put the arguements into
* 			nbytes -- number of bytes to copy
*	output: returns 0 on success, -1 on failure
*	effect: changes contents of buf
*/
int32_t getargs (uint8_t* buf, int32_t nbytes) {
	/* Check to see if buf is NULL or not */
	if (buf == NULL)
		return -1;
	/* Get our pcb pointer we are making system call in */
	pcb_t* pcb = get_pcb_ptr();
	/* Copy into the arguf */
	strcpy((int8_t*)buf, (int8_t*)pcb->argbuf);
	return 0;
}


/* 
*	Function vidmap()
*	Description: maps the text-mode video memory into user space at a pre-set virtual address
*	input: 	screen_start -- pointer to start of user video memory
*	output: returns virtual address on success, -1 on failure
*	effect: effects the physical video memory buffer 
*/
int32_t 
vidmap(uint8_t ** screen_start){
	/* Map Virtual Address to Physical Address */
	if (screen_start == NULL || screen_start == (uint8_t**)_4MB)
	{
		return -1;
	}
	remapWithPageTable((uint32_t)_136MB, (uint32_t)VIDEO);
	*screen_start = (uint8_t*)_136MB;

	return _136MB;
}


/* 
*	Function set_handler()
*	NOT IMPLEMENTED
*/
int32_t 
set_handler(int32_t signum, void* handler_address){
	/* EXTRA CREDIT - NOT IMPLEMENTED */
	return -1;
}


/* 
*	Function sigreturn()
*	NOT IMPLEMENTED
*/
int32_t 
sigreturn(void){
	/* EXTRA CREDIT - NOT IMPLEMENTED*/
	return -1;
}

/* 
*	Function get_available_process_number()
*	Description: gets the next available process number using the process id array
*	input: none
*	output: returns the next available process number upon success, -1 upon failure
*	effect: none
*/
int32_t get_available_process_number()
{
	/* Determine the next available process number */
    int32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) 
    {
        if (process_id_array[i] == 0) 
        {
        	process_id_array[i] = 1;
	    	return i;
        }
    }
    /* Return -1 if there is no more processes available */
    printf("Too many processes running. ");
    return -1;
}

/*
*	Function get_pcb_ptr()
*	Description: gets a pointer to the current pcd struct
*	input: none
*	output: pcb pointer
*	effect: none
*/
pcb_t* get_pcb_ptr(void){
	pcb_t* ptr;
	asm volatile("				   \n\
				andl %%esp, %%eax  \n\
				"
				:"=a"(ptr)
				:"a"(PCB_PTR_MASK)
				:"cc"
				);
	return ptr;
};

/*
*	Function get_pcb_ptr()
*	Description: gets a pointer to the current pcd struct
*	input: none
*	output: pcb pointer
*	effect: none
*/
pcb_t * get_pcb_ptr_process(uint32_t process)
{
	return (pcb_t *)(_8MB - (process + 1) * _8KB);
}


/*
*	Function failure_function()
*	Description: does nothing and returns -1
*	input: none
*	output: returns -1
*	effect: none
*/
int32_t failure_function()
{
    return -1;
}
