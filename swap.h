#ifndef SWAP_H
#define SWAP_H

#include "system.h"

/**
 * Lab7:vmem
 * useful utility funcs for paging
 */

// Fetch a page from the virtual mem with a specific sector num
extern void vmFetch(int secNo, char* data);

// Write a page into the virtual mem with a specific sector num
extern void vmWrite(int secNo, char* data);

// swap vmem pages in or swap phys pages out or both
// depending on the page-replacement algorithm
extern void swapPage(int vAddr);

#endif //SWAP_H
