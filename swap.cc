#include "swap.h"


void physMemCopy(char* src,char* dest,int size,int sPos,int dPos){
    for(int i=0;i<size;i++){
        dest[dPos+i] = src[sPos+i];
    }
}

void vmFetch(int secNo,char* data){
    swap->ReadSector(secNo,data);
}

void vmWrite(int secNo,char* data){
    swap->WriteSector(secNo,data);
}

/**
 * Lab7: vmem
 * @param vAddr provided by the caller,calculating based on the badVAddrReg
 */
void swapPage(int vAddr){
    TranslationEntry* vmpt = machine->pageTable;
    // step 1: find a victim by replacement algorithm
    int rPage; // rPage is short for replaced page
    rPage = calVictim();

    // step 2: check if victim is dirty(need to write back)
    if(vmpt[rPage].dirty){
        // step 2.1: write the dirty page back to the swap space
        char dirtyData[PageSize];
        // copy the dirty data from main mem
        physMemCopy(machine->mainMemory,dirtyData,PageSize,vmpt[rPage].physicalPage*PageSize,0);
        // write it back to the swap disk
        vmWrite(vmpt[rPage].vMemPage,dirtyData);
    }

    // step 3: get the page no. of vAddr in the swap space
    int logPage = vAddr/PageSize; // log is short for logical
    int tPage = vmpt[logPage].vMemPage; // t is short for target

    // step 4: Fetch the page in the swap space
    char buf[PageSize];
    vmFetch(tPage,buf);

    // step 5: set the valid bits of both rPage and logPage,also the dirty bit
    vmpt[rPage].valid = False;
    vmpt[logPage].valid = True;
    vmpt[rPage].dirty = False;

    // step 6: rewrite the data in the buffer back to the physical main mem
    physMemCopy(buf,machine->mainMemory,PageSize,0,vmpt[rPage].physicalPage*PageSize);

    // step 7: update the nachos virtual mem page table
    vmpt[logPage].physicalPage = vmpt[rPage].physicalPage;
    vmpt[rPage].physicalPage = IllegalPhysPage;
}

