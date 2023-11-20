// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "swap.h"

extern void printMainMemory(int sa,int size);

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}


//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    //DEBUG('u',"ready to allocate user process mem\n");
    //printf("ASID: %d\n",ASID);
    // allocate an asId
    asId = ASID++;
    printf("spaceId:%d\n",asId);

    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    /**
     * Lab6:mup
     * the numPages should not be greater than the number of
     * idle pages on the page bit map
     */
    ASSERT((int)numPages <= pageMap->NumClear());
    /**
     * Lab7:vmem
     * make sure there are enough space on the swap disk
     */
    ASSERT((int)numPages <= vmMap->NumClear());

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
    // first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        /**
         * Lab7:vmem
         * pure demand paging
         */
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = IllegalPhysPage;
        pageTable[i].vMemPage = vmMap->Find();
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on a separate page, we could set its pages to be read-only
    }
    numInUse = 0;
    refStk = new int[maxInUse];
    for(int i=0;i<maxInUse;i++){
        refStk[i] = -1;
    }
    stkSize = 0;

#ifdef OPT
    refString = new int[maxInUse];
    refStrLen = 0;
    maxLen = maxInUse;
    lastVPN = -1;
#endif
    Print();
    
    // zero out the entire address space, to zero the uninitialized data segment
    // and the stack segment
    //bzero(machine->mainMemory, size);

    /**
     * Lab7:vmem
     * Initially we load the noff file to the swap disk
     * when there's a page fault
     * fetch the data on the disk to main mem
     */
     char buf[size];
     bzero(buf,size);
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(buf[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(buf[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
    // load the buf to the swap disk
    for(int i=0;i<numPages;i++){
        vmWrite(pageTable[i].vMemPage,&buf[i*PageSize]);
    }
    if(false){
        // for debugging the contents we wrote into the swap space
        // if you want to test this
        // plz change the code above to allocate some phys pages to this as
        // such as:pageTable[i].physicalPage = pageMap->Find();
        //
        vmMap->Print();
        int pageStart,frameStart,pOffset;
        if (noffH.code.size > 0) {
            pageStart = noffH.code.virtualAddr/PageSize;
            pOffset = noffH.code.virtualAddr % PageSize;
            frameStart = pageTable[pageStart].physicalPage;
            executable->ReadAt(&(machine->mainMemory[frameStart*PageSize+pOffset]),
                               noffH.code.size, noffH.code.inFileAddr);
        }
        if (noffH.initData.size > 0) {
            pageStart = noffH.initData.virtualAddr/PageSize;
            pOffset = noffH.initData.virtualAddr % PageSize;
            frameStart = pageTable[pageStart].physicalPage;
            executable->ReadAt(&(machine->mainMemory[frameStart*PageSize+pOffset]),
                               noffH.initData.size, noffH.initData.inFileAddr);
        }
        pageStart = noffH.code.virtualAddr/PageSize;
        pOffset = noffH.code.virtualAddr % PageSize;
        frameStart = pageTable[pageStart].physicalPage;
        printf("noff file size:%d\n",size);
        for(int i=0;i<size;i++){
            bool flag = buf[i]==machine->mainMemory[frameStart*PageSize+pOffset+i];
            printf("%d",flag?1:0);
        }
        printf("\n");
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // free the bit map
    for(int i=0;i<numPages;i++){
        if(pageTable[i].valid){
            pageMap->Clear(pageTable[i].physicalPage);
        }
        vmMap->Clear(pageTable[i].vMemPage);
    }
    delete[] refStk;
#ifdef OPT
    delete[] refString;
#endif
   delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::Print() {
    printf("SpaceId: %d, Page table dump: %d pages in total\n", asId,numPages);
    printf("====================================================================================================================================\n");
    printf("\tVirtPage, \tPhysPage\tVMPage\t\tValid\t\tUse\t\tDirty\n");
    for (int i=0; i < (int)numPages; i++) {
        printf("\t%d, \t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
               pageTable[i].virtualPage, pageTable[i].physicalPage,pageTable[i].vMemPage,
               pageTable[i].valid?1:0,pageTable[i].use?1:0,pageTable[i].dirty?1:0
               );
    }
    printf("====================================================================================================================================\n\n");
}

int AddrSpace::getAsId() {
    return asId;
}

/**
 * Lab7:vmem
 * @param refPage virtual/logical page need to be updated when there's no page fault exception
 */
void
AddrSpace::updateRefStk(int refPage) {
    int target;
    for(target=0;target<stkSize;target++){
        if(refStk[target]==refPage){
            break;
        }
    }
    ASSERT(refStk[target]==refPage);
    int tPage = refStk[target];
    for(int i=target+1;i<stkSize;i++){
        refStk[i-1] = refStk[i];
    }
    refStk[stkSize-1]=tPage;
    if(DebugIsEnabled('r')){
        printf("update ref stack state: ");
        for(int i=0;i<stkSize;i++){
            printf("%d ",refStk[i]);
        }
        printf("\n");
    }
}

#ifdef OPT
/**
* Lab7:vmem
 * update ref str for OPT algorithm evaluation
*/
void
AddrSpace::updateRefStr(int refPage) {
    // unnecessary to record two same contiguous refs
    // if the first one has been loaded to mem
    // then the second one also did
    if(lastVPN == refPage){
        return;
    }
    lastVPN = refPage;
    //DEBUG('A',"refStrLen: %d\n",refStrLen);
    // if there's still enough space in the refStr arr
    // just use it
    if(refStrLen<maxLen){
        refString[refStrLen++] = refPage;
        return;
    }
    // Otherwise,create a new arr to replace the cur one
    maxLen*=2;
    int* narr = new int[maxLen];
    for(int i=0;i<refStrLen;i++){
        narr[i] = refString[i];
    }
    narr[refStrLen++] = refPage;
    delete[] refString;
    refString = narr;
}

/**
* Lab7:vmem
 * calculate page faults and write back for OPT algorithm
*/
int
AddrSpace::calOPTPf() {
    if(DebugIsEnabled('A')){
        printf("len: %d,ref str: ",refStrLen);
//        for(int i=0;i<refStrLen;i++){
//            printf("%d ",refString[i]);
//        }
//        printf("\n");
    }
    int ans = 0;
    int opt[maxInUse];
    int optSize = 0;
    for(int i=0;i<maxInUse;i++){
        opt[i] = -1; // vpn will never be -1
    }
    int victim,max,cmp;
    for(int i=0;i<refStrLen;i++){
        // check if vpn is already in the mem
        int ref = refString[i];
        //DEBUG('A',"cur ref: %d\n",ref);
        bool flag = false;
        for(int j=0;j<maxInUse;j++){
            if(opt[j] == ref){
                flag = true;
                break;
            }
        }
        if(!flag){
            ans++; // not in the mem
            // still enough space?
            if(optSize<maxInUse){
                opt[optSize++] = ref;
                continue;
            }
            // need paging,find victim
            victim = 0;
            max = 0;
            for(int j=0;j<maxInUse;j++){
                cmp = 0;
                for(int k= i+1;k<refStrLen;k++){
                    if(refString[k]!=opt[j]){
                        cmp++;
                    }else{
                        break;
                    }
                }
                if(cmp>max){
                    max = cmp;
                    victim = j;
                }
            }
            // swap the victim and the vpn
            opt[victim] = ref;
        }
    }
    return ans;
}
#endif

