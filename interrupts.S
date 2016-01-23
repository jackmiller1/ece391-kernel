#	interrupts.S - Assembly Wrapper for Interrupt Handlers
# 	This piece of code takes care of the interrupt handler
# 	and calls the appropriate function to handle the interrupt
#define ASM 1
#include "x86_desc.h"

.global system_call_handler

#define HANDLER(name,send_to_fn)			\
.GLOBL name									;\
name:										;\
	pushal									;\
	pushfl									;\
	call send_to_fn							;\
	popfl									;\
	popal									;\
	iret									;\
	
# keyboard_handler: interrupt handler for keyboard interrupts
HANDLER(keyboard_handler, keyboard_interrupt_handler);
# clock_handler: interrupt handler for rtc interrupts
HANDLER(rtc_handler, rtc_interrupt_handler);
# pit handler: interrupt handler for pit interrupts
HANDLER(pit_handler, PIT_interrupt_and_schedule);

#-------------------------------------------------------------------#

#System Call Handler
#Save Registers -> Push Arguments -> Check Validity -> 
#     Load Call -> Make Call -> Restore Registers -> Interrupt Return.

#SYSTEM CALL JUMP TABLE - ONLY 1 - 6 ("execute" -> "close") FOR CHECKPT 2
system_call_jump_table:
	.long 0x0, halt, execute, read, write, open, close, getargs, vidmap

# Main Syscall Handler
system_call_handler:
	# Save all registers and flags except for eax, the return value
	cli
	pushl %es
  	pushl %ds
  	pushl %ebx
  	pushl %ecx
  	pushl %edx
  	pushl %esi
  	pushl %edi
  	pushl %ebp
  	pushfl

  	# Pushing arguments - need to save all registers according to Appendix B.
  	pushl %ebp		#Pushed "to avoid leaking information to the user programs"
  	pushl %edi		#Pushed "to avoid leaking information to the user programs"
  	pushl %esi		#Pushed "to avoid leaking information to the user programs"
  	pushl %edx		#Argument 3
  	pushl %ecx 		#Argument 2
  	pushl %ebx		#Argument 1

  	#Check to see if our System Call Number (stored in %EAX) is within bounds (Chkpt 3 - 1:6)
  	cmpl $1, %eax
  	jl invalid
  	cmpl $8, %eax
  	jg invalid
	
	# Call the correct system call according to the jumptable
	sti
  	call *system_call_jump_table(,%eax,4)
	cli
  	jmp restore

invalid:
 	movl $-1, %eax

restore:
  	# Popping arguments - 6 Registers * 4 Bytes = 24
  	addl $24, %esp

  	# Restore all regs, except for eax, and flags
  	popfl
  	popl %ebp
  	popl %edi
  	popl %esi
  	popl %edx
 	popl %ecx
  	popl %ebx
  	popl %ds
  	popl %es
	sti
  	# Return from interrupt
  	iret



