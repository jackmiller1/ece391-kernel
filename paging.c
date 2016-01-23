/*
*   paging.c - used to initialize paging
*/

#include "paging.h"
#include "types.h"

//global array for page directory
// other option is to make it uint32_t
uint32_t pageDirectory[ONEKILO] __attribute__((aligned(FOURKILO)));

//global array for page table
uint32_t pageTable[ONEKILO] __attribute__((aligned(FOURKILO)));

//global array for user page table
uint32_t userPageTable[ONEKILO] __attribute__((aligned(FOURKILO)));

//global array for user page table
uint32_t vidMemPageTable[ONEKILO] __attribute__((aligned(FOURKILO)));


/*
*   Function: init_paging()
*   description: This function should be called in the entry function of kernel.c. It initilizes the
*                page directory and page table arrays and enables paging by setting the cr3, cr4, cr0 regs.
*                Memory is mapped one to one for the kernel (4 meg chunk) and video memory (4KB page) and
*                no other memory is "present".
*   inputs: none
*   outputs: none
*
*/
void 
init_paging(){
    //initialize page directory and page table entries to default values
    int i;
    for(i = 0; i < ONEKILO; i++){
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        pageDirectory[i] = 0x00000002;
        
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes
        pageTable[i] = (i * FOURKILO) | 2; // attributes: supervisor level, read/write, not present.
    }
    
    // add page table to page directory
    // attributes: supervisor level, read/write, present
    pageDirectory[0] = ((unsigned int)pageTable) | 3;
    // map second entry to 4MB for Kernel
    pageDirectory[1] = FOURMEG | 0x83; //attributes: supervisor, present, r/w, size (set to 1 for 4MB page)
    // create a page table entry for video memory (location 0xB8000 found in lib.c)
    pageTable[0xB8] |= 3; // attributes: supervisor level, read/write, present.
    
    //turn on paging
    asm volatile(
                 "movl %0, %%eax;"
                 "movl %%eax, %%cr3;"
                 "movl %%cr4, %%eax;"
                 "orl $0x00000010, %%eax;"
                 "movl %%eax, %%cr4;"
                 "movl %%cr0, %%eax;"
                 "orl $0x80000000, %%eax;"
                 "movl %%eax, %%cr0;"
                 :                      /* no outputs */
                 :"r"(pageDirectory)    /* input */
                 :"%eax"                /* clobbered register */
                 );
}

/*
 *   Function: remap
 *   description: This function takes virtutal and physical addresses (virtual must be a multiple of 4MB) and maps the 4MB chunk of memory (a single PDE)
 *                begining at the given virtual address to the given physical address
 *   inputs: virtualAddr - multiple of 4MB virtual address to be mapped
 *           physicalAddr - physical address to be mapped
 *   outputs: none
 *
 */
void remap(uint32_t virtualAddr, uint32_t physicalAddr)
{
    uint32_t pde = virtualAddr / FOURMEG;
    pageDirectory[pde] = physicalAddr | 0x87; //attributes: user, present, r/w, size (set to 1 for 4MB page)
    flush_tlb();
}

/*
 *   Function: remapWithPageTable
 *   description: This function takes virtutal and physical addresses (virtual must be a multiple of 4MB) and maps the 4MB chunk of memory (a single PDE)
 *                begining at the given virtual address to the first page of the user page table
 *   inputs: virtualAddr - multiple of 4MB virtual address to be mapped
 *           physicalAddr - physical address to be mapped
 *   outputs: none
 *
 */
void remapWithPageTable(uint32_t virtualAddr, uint32_t physicalAddr)
{
    uint32_t pde = virtualAddr / FOURMEG;
    // attributes: user level, read/write, present
    pageDirectory[pde] = ((unsigned int)userPageTable) | 7;
    userPageTable[0] = physicalAddr | 7; // attributes: user, read/write, present
    flush_tlb();
}

/*
 *   Function: remapVideoWithPageTable
 *   description: This function takes virtutal and physical addresses (virtual must be a multiple of 4MB) and maps the 4MB chunk of memory (a single PDE)
 *                begining at the given virtual address to the first page of the video page table
 *   inputs: virtualAddr - multiple of 4MB virtual address to be mapped
 *           physicalAddr - physical address to be mapped
 *   outputs: none
 *
 */
void remapVideoWithPageTable(uint32_t virtualAddr, uint32_t physicalAddr)
{
    uint32_t pde = virtualAddr / FOURMEG;
    // attributes: user level, read/write, present
    pageDirectory[pde] = ((unsigned int)vidMemPageTable) | 7;
    vidMemPageTable[0] = physicalAddr | 7; // attributes: user, read/write, present
    flush_tlb();
}

/*
 *   Function: remapWithPageTableToPage
 *   description: This function takes virtutal and physical addresses (virtual must be a multiple of 4MB) and maps the 4MB chunk of memory (a single PDE)
 *                begining at the given virtual address to the specified page of the user page table
 *   inputs: virtualAddr - multiple of 4MB virtual address to be mapped
 *           physicalAddr - physical address to be mapped
 *           page - the page in the page table that we want to map to
 *   outputs: none
 *
 */
void remapWithPageTableToPage(uint32_t virtualAddr, uint32_t physicalAddr, uint32_t page)
{
    uint32_t pde = virtualAddr / FOURMEG;
    // attributes: user level, read/write, present
    pageDirectory[pde] = ((unsigned int)userPageTable) | 7;
    userPageTable[page] = physicalAddr | 7; // attributes: user, read/write, present
    flush_tlb();
}


/*
 *   Function: flush_tlb
 *   description: This reloads the tlb
 *   inputs: none
 *   outputs: none
 *
 */
void flush_tlb(void){
	asm volatile(
                 "mov %%cr3, %%eax;"
                 "mov %%eax, %%cr3;"
                 :                      /* no outputs */
                 :                      /* no inputs */
                 :"%eax"                /* clobbered register */
                 );
}
