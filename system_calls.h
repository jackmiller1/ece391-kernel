/*
*	system_calls.h - Function Header File to be used with system_calls.c
*/
#ifndef _SYSTEM_CALLS_H
#define _SYSTEM_CALLS_H

#include "types.h"
#include "terminal.h"

#define PCB_PTR_MASK 0xFFFFE000 
#define LARGENUMBER 100000
#define LOAD_ADDRESS 0x8048000
#define IN_USE 0x0001
#define NOT_IN_USE 0x0000
#define FILE_START 0x0000

#define RTC_TYPE	0 
#define DIR_TYPE	1
#define	FILE_TYPE	2

#define MIN_FD		2
#define MAX_FD		7   

#define MAX_FILES 8
#define MAX_PROCESSES 6

#define FILE_NAME_SIZE 32
#define MAX_COMMAND_SIZE 10
#define MAX_BUFFER_SIZE 100
#define READ_BUFFER_SIZE 4
#define ENTRY_POINT_START 24

/*** Struct: fops_table
*     Read: function pointer to a specific read function
*	  Write: function pointer to a specific write function
* 	  Open: function pointer to a specific open function
*	  Close: function pointer to a specific close function
***/
 typedef struct {
	 int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
	 int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
	 int32_t (*open)(const uint8_t* filename);
	 int32_t (*close)(int32_t fd);
 } fops_table;
 
 
/*** Struct: file_desc_t
*    jumptable - pointer to a file operations table for this file (open, close, read, and write.)
*    inode - inode number of this file in the file system. 
*    file_position - current position within the file that we are reading. increment as we read it. 
*    flags - used to figure out which fds are available for use when trying 
***/ 
typedef struct { 
	fops_table fops_table_ptr; 
	int32_t inode; 
	int32_t file_position; 
	int32_t flags; 
} file_desc_t;

/*** Struct: pcb_t
*    fds[8] - array of file descriptors - contain each file that the process has open. 
*    filenames[8][32] - array holding the names of each of the open files 
*    parent_ksp, parent_kbp - The kernel stack & base pointer of the parent process. Used upon halt
*    process_number, parent_process_number - Process number of this process. Number from 1-7.
*    argbuf - Buffer for the arguments of this process.  
*    ksp_before_change, kbp_before_change - This variable stores the KSP, KBP right before switching processes.  
***/ 
typedef struct { 
	file_desc_t fds[MAX_FILES]; 
	uint8_t filenames[MAX_FILES][FILE_NAME_SIZE];  
	uint32_t parent_ksp; 
	uint32_t parent_kbp; 
	uint8_t process_number; 
	uint8_t parent_process_number; 
	int8_t argbuf[MAX_BUFFER_SIZE]; 
	term_t * term;
    uint32_t esp;
    uint32_t ebp;
 } pcb_t; 
 
 extern uint8_t process_id_array [MAX_PROCESSES];

/* Halt System Call */
int32_t halt (uint8_t status);

/* Execute System Call */
int32_t execute (const uint8_t* command);

/* Read System Call */
int32_t read (int32_t fd, void* buf, int32_t  nbytes);

/* Write System Call */
int32_t write (int32_t fd, const void* buf, int32_t nbytes);

/* Open System Call */
int32_t open (const uint8_t* filename);

/* Close System Call */
int32_t close (int32_t fd);

/* Getargs System Call */
int32_t getargs (uint8_t* buf, int32_t nbytes);

/* Vidmap System Call */
int32_t vidmap (uint8_t** screen_start);

/* Set Handler System Call */
int32_t set_handler (int32_t signum, void* handler_address);

/* Sigreturn System Call */
int32_t sigreturn (void);

/* Gets current PCB pointer */
pcb_t* get_pcb_ptr();

pcb_t* get_pcb_ptr_process(uint32_t process);

/* Gets next available process number */
int32_t get_available_process_number();

/* Return -1 function */
int32_t failure_function();


#endif
