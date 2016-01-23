/*
*	interrupt_table.c - used to initialize IDT and allow interrupts
*/
#include "interrupt_table.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "interrupts.h"

#define SYSCALL_VECTOR		0x80
#define RTC_VECTOR			0x28
#define KEYBOARD_VECTOR		0x21
#define PIT_VECTOR 			0x20

/* Exceptions sent to the exception creator macro.
*  IDT Table initialized using reference to:
*		http://phrack.org/issues/59/4.html
*	as well on pg. 145/838 in IA-32 Software Dev Manual
*/
EXCEPTION_THROWN(DIVIDE_EXCEPTION,"Divide Error");
EXCEPTION_THROWN(DEBUG_EXCEPTION,"Debug Exception");
EXCEPTION_THROWN(NMI_EXCEPTION,"Non Maskable Interrupt Exception");
EXCEPTION_THROWN(INT3_EXCEPTION,"Breakpoint Exception");
EXCEPTION_THROWN(OVERFLOW_EXCEPTION,"Overflow Exception");
EXCEPTION_THROWN(BOUNDS_EXCEPTION,"BOUND Range Exceeded Exception");
EXCEPTION_THROWN(INVALID_OPCODE_EXCEPTION,"Invalid Opcode Exception");
EXCEPTION_THROWN(DEVICE_NOT_AVAILABLE_EXCEPTION,"Device Not Available Exception");
EXCEPTION_THROWN(DOUBLE_FAULT_EXCEPTION,"Double Fault Exception");
EXCEPTION_THROWN(COPROCESSOR_SEGMENT_OVERRUN_EXCEPTION,"Coprocessor Segment Exception");
EXCEPTION_THROWN(TSS_EXCEPTION,"Invalid TSS Exception");
EXCEPTION_THROWN(SEG_NOT_PRESENT_EXCEPTION,"Segment Not Present");
EXCEPTION_THROWN(STACK_SEGMENT_EXCEPTION,"Stack Fault Exception");
EXCEPTION_THROWN(GENERAL_PROTECTION_EXCEPTION,"General Protection Exception");
EXCEPTION_THROWN(PAGE_FAULT_EXCEPTION,"Page Fault Exception");
/**15 - RESERVED BY INTEL (?) */
EXCEPTION_THROWN(FLOAT_EXCEPTION,"Floating Point Exception");
EXCEPTION_THROWN(ALIGN_CHECK_EXCEPTION,"Alignment Check Exception");
EXCEPTION_THROWN(MACHINE_CHECK_EXCEPTION,"Machine Check Exception");


/* Function: general_interruption()
*  Description: this is a general interrupt that is not defined
*  in our IDT table explicitely (i.e. 0-18 exceptions)
*  inputs: none
*  ouputs: none
*  effects: masks interrupts, prints to screen, unmasks interrupts
*/
void 
general_interruption(void) 
{
	cli();
	printf("Undefined interruption!");
	sti();
}

/*	Function: init_interrupts()
*	Description: intializes the interrupts upon boot
*	input: none
*	output: 0 if successful, 1 if unsuccessful
	effects: sets the IDT entries to be the associated exceptions
*/
int 
init_interrupts(void)
{
   /* Initialize index */
   int i = 0;
   /* Loop through IDT Vector table (according to NUM_VEC) and
   *  define the specific interrupt vector to the according exception */
   for(; i < NUM_VEC; i++) {
    	/* SET INTERRPUT VECTOR AS FOLLOWS:
		*
		* typedef union idt_desc_t {
		*		uint16_t offset_15_00;	== Offset (15...0) = Don't Touch
		*		uint16_t seg_selector;  == Segment Selector = KERNEL_CS
		*		uint8_t  reserved4 :	== Reserved Bit 4
		*		uint32_t reserved3 :	== Reserved Bit 3
		*		uint32_t reserved2 :	== Reserved Bit 2
		*		uint32_t reserved1 :	== Reserved Bit 1
		*		uint32_t size : 1;		== Size of gate (D): 1 = 32 bits; 0 = 16 bits
		*		uint32_t reserved0 : 1;	== Reserved Bit 0
		*		uint32_t dpl : 2;		== Descriptor Privilege Level = 0x3-User, 0x0-Kernel
		*		uint32_t present : 1;  	== Present Bit (must be set to 1 to utilize)
		*		uint16_t offset_31_16;	== Offset (31...16)
		* } idt_desc_t;
		*
		*	TRAP (EXCEPTION) Gate: 		R0 = 0, R1 = 1, R2 = 1, R3 = 0, R4 = 0
		*	INTERRUPT Gate: 			R0 = 0, R1 = 1, R2 = 1, R3 = 1, R4 = 0
		*	SYSTEM_CALL :				R0 = 0, R1 = 1, R2 = 1, R3 = 1, R4 = 0
		*
		*/

		idt[i].present = 0x1;
    	idt[i].dpl = 0x0;
  		idt[i].size = 0x1;
		//General Interrupt = R0R1R2R3R4 = 01100 (32 -> 256)
		idt[i].reserved0 = 0x0;
  		idt[i].reserved1 = 0x1;
  		idt[i].reserved2 = 0x1;
  		idt[i].reserved3 = 0x0;
  		idt[i].reserved4 = 0x0;
  		idt[i].seg_selector = KERNEL_CS;

		//General Exception = R0R1R2R3R4 = 01110 (0->32)
  		if(i < 32){
    		idt[i].reserved3 = 0x1;
  		}

		//System Call = Same as Exception R0R1R2R3R4 = 01110 (0->32)
		// However, this will come from DPL = 3 (User)
  		if(SYSCALL_VECTOR == i)
		{
			idt[i].reserved3 = 1;
			idt[i].dpl = 0x3;
		}
    		
	}

	/* Define INT 0x00 through INT 0x18. Route them to respective handlers. */
	SET_IDT_ENTRY(idt[0], DIVIDE_EXCEPTION);
	SET_IDT_ENTRY(idt[1], DEBUG_EXCEPTION);
	SET_IDT_ENTRY(idt[2], NMI_EXCEPTION);
	SET_IDT_ENTRY(idt[3], INT3_EXCEPTION);
	SET_IDT_ENTRY(idt[4], OVERFLOW_EXCEPTION);
	SET_IDT_ENTRY(idt[5], BOUNDS_EXCEPTION);
	SET_IDT_ENTRY(idt[6], INVALID_OPCODE_EXCEPTION);
	SET_IDT_ENTRY(idt[7], DEVICE_NOT_AVAILABLE_EXCEPTION);
	SET_IDT_ENTRY(idt[8], DOUBLE_FAULT_EXCEPTION);
	SET_IDT_ENTRY(idt[9], COPROCESSOR_SEGMENT_OVERRUN_EXCEPTION);
	SET_IDT_ENTRY(idt[10], TSS_EXCEPTION);
	SET_IDT_ENTRY(idt[11], SEG_NOT_PRESENT_EXCEPTION);
	SET_IDT_ENTRY(idt[12], STACK_SEGMENT_EXCEPTION);
	SET_IDT_ENTRY(idt[13], GENERAL_PROTECTION_EXCEPTION);
	SET_IDT_ENTRY(idt[14], PAGE_FAULT_EXCEPTION);
	//Interrupt Vector #15 - RESERVED BY INTEL
	SET_IDT_ENTRY(idt[16], FLOAT_EXCEPTION);
	SET_IDT_ENTRY(idt[17], ALIGN_CHECK_EXCEPTION);
	SET_IDT_ENTRY(idt[18], MACHINE_CHECK_EXCEPTION);
	
	/*RTC Interrupt Handler - start in interrupts.S */
	SET_IDT_ENTRY(idt[RTC_VECTOR], rtc_handler);

	/*Keyboard Interrupt Handler - start in interrupts.S */
	SET_IDT_ENTRY(idt[KEYBOARD_VECTOR], keyboard_handler);
	
	/*Keyboard Interrupt Handler - start in interrupts.S */
	SET_IDT_ENTRY(idt[PIT_VECTOR], pit_handler);

	/*System Call Interrupt Handler - start in interrupts.S */
	SET_IDT_ENTRY(idt[SYSCALL_VECTOR], system_call_handler);
   // Load the IDT.
   lidt(idt_desc_ptr);
   return 0;
 }
