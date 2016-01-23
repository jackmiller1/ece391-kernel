/*
*	paging.h - Function Header File to be used with paging.c
*/

#include "types.h"
#define ONEKILO 1024
#define FOURKILO 4096
#define FOURMEG 0x400000


//data structures
extern uint32_t pageDirectory[1024] __attribute__((aligned(4096)));
extern uint32_t pageTable[1024] __attribute__((aligned(4096)));


/* Function Definitions */
void init_paging();
void remap(uint32_t virtualAddr, uint32_t physicalAddr);
void remapWithPageTable(uint32_t virtualAddr, uint32_t physicalAddr);
void remapVideoWithPageTable(uint32_t virtualAddr, uint32_t physicalAddr);
void remapWithPageTableToPage(uint32_t virtualAddr, uint32_t physicalAddr, uint32_t page);
void flush_tlb(void);

