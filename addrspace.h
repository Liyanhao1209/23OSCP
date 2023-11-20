// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "list.h"

extern int ASID;

#define UserStackSize		1024 	// increase this as necessary!
#define maxInUse 5

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch

    /**
     * Lab6:mup
     * for debugging
     */
    void Print();

    // getter of asId
    int getAsId();

    // getter of pageTable
    TranslationEntry* getPT(){
        return pageTable;
    }

    // getter of numPages
    unsigned int getNumPages(){
        return numPages;
    }

    /**
     * Lab7:vmem
     * LRU replacement algorithm required
     */
     int* refStk;
     int stkSize;
     int numInUse;

#ifdef OPT
     /**
      * Lab7:vmem extension
      * record the ref string of user prog
      */
      int* refString;
      int refStrLen;
      int maxLen;
      int lastVPN;

      void updateRefStr(int refPage);

     int calOPTPf();
#endif

     void updateRefStk(int refPage);



  private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual address space

    int asId; // address space id to identify various user process addr
};

#endif // ADDRSPACE_H
