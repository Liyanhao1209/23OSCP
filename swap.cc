#include "swap.h"


void physMemCopy(char* src,char* dest,int size,int sPos,int dPos){
    //printf("physMemCopy start...\n");
    for(int i=0;i<size;i++){
        dest[dPos+i] = src[sPos+i];
    }
    //printf("physMemCopy end...\n");
}

void vmFetch(int secNo,char* data){
    swap->ReadSector(secNo,data);
}

void vmWrite(int secNo,char* data){
    swap->WriteSector(secNo,data);
}

// write the dirty page back to the swap space
void dirty2vm(int dirtyPage,TranslationEntry* vmpt){
    printf("dirty page: %d write back\n",dirtyPage);
    char dirtyData[PageSize];
    // copy the dirty data from main mem
    physMemCopy(machine->mainMemory,dirtyData,PageSize,vmpt[dirtyPage].physicalPage*PageSize,0);
    // write it back to the swap disk
    vmWrite(vmpt[dirtyPage].vMemPage,dirtyData);
    stats->numVictimWriteBack++;
}


// Fetch data in vm space to main mem
int vm2Mem(int vAddr,TranslationEntry* vmpt,int desPos){
    // step 1: get the page no. of vAddr in the swap space
    int logPage = vAddr/PageSize; // log is short for logical
    int tPage = vmpt[logPage].vMemPage; // t is short for target

    // step 2: Fetch the page in the swap space
    char buf[PageSize];
    DEBUG('v',"the page caused page fault is : %d,target vmem page number is : %d\n",logPage,tPage);
    vmFetch(tPage,buf);
//    if(DebugIsEnabled('v')){
//        printf("buf content: \n");
//        for(int i=0;i<PageSize;i++){
//            printf("%c",buf[i]);
//        }
//        printf("\n");
//    }

    // step 3: rewrite the data in the buffer back to the physical main mem
    physMemCopy(buf,machine->mainMemory,PageSize,0,desPos);

    // return the logical page of vAddr
    return logPage;
}

/**
 * Lab7: vmem
 * @param vAddr provided by the caller,calculating based on the badVAddrReg
 * @return the logical page cause page fault
 */
int swapPage(int vAddr){
    TranslationEntry* vmpt = machine->pageTable;
    // step 1: find a victim by replacement algorithm
    int rPage; // rPage is short for replaced page
    rPage = calVictim();
    DEBUG('s',"swap victim: %d\n",rPage);

    // step 2: check if victim is dirty(need to write back)
    if(vmpt[rPage].dirty){
        dirty2vm(rPage,vmpt);
    }

    int logPage = vm2Mem(vAddr,vmpt,vmpt[rPage].physicalPage*PageSize);

    // step 3: set the valid bits of both rPage and logPage,also the dirty bit
    vmpt[rPage].valid = false;
    vmpt[logPage].valid = true;
    vmpt[rPage].dirty = false;

    // step 4: update the nachos virtual mem page table
    vmpt[logPage].physicalPage = vmpt[rPage].physicalPage;
    vmpt[rPage].physicalPage = IllegalPhysPage;

    printf("Swap page %d out, demand page %d in frame(%d)\n",rPage,logPage,vmpt[logPage].physicalPage);

    return logPage;
}

