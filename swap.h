#ifndef SWAP_H
#define SWAP_H

#include "system.h"
#include "replace.h"

#define IllegalPhysPage -1
/**
 * Lab7:vmem
 * useful utility funcs for paging
 */

// Fetch a page from the virtual mem with a specific sector num
extern void vmFetch(int secNo, char* data);

// Write a page into the virtual mem with a specific sector num
extern void vmWrite(int secNo, char* data);

// only occurs when the user prog ran out of its allocated frames(5)
// depending on the page-replacement algorithm
extern void swapPage(int vAddr);

#endif //SWAP_H
