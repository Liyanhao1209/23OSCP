#include "swap.h"

void physMemCopy(char* dest,int size){

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
    //check if this page is dirty(need to write back)
    int logicalPage = vAddr/PageSize;
    if(vmpt[logicalPage].dirty){
        vmWrite(vmpt[logicalPage].vMemPage,vmpt[logicalPage].physicalPage)
    }
}

