// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "swap.h"
#include "replace.h"
#include "addrspace.h"

/**
 * Lab6:mup
 * From contents for the ExceptionHandler:
 * And don't forget to increment the pc before returning. (Or else you'll
 * loop making the same system call forever!
 * since the registers field is public,just get access to it!
 */
void IncPc(){
    machine->registers[PrevPCReg] =  machine->registers[PCReg];
    machine->registers[PCReg] = machine->registers[NextPCReg];
    machine->registers[NextPCReg] += 4;
}

/**
 * Lab7: vmem
 * load the page in the swap place to physical mem
 * use func:swapPage or not depends there are still enough spaces for the refStk
 */
void loadPage(int vAddr){
    // step1: check if there is enough space for the refStk
    AddrSpace* cas = currentThread->space;
    DEBUG('a',"current addr space id: %d\n",cas->getAsId());
    int logPage;
    DEBUG('u',"current user prog frame number-in-use: %d\n",cas->numInUse);
    if(cas->numInUse < maxInUse){
        TranslationEntry* vmpt = machine->pageTable;
        // make sure there are enough pages on the page map
        ASSERT(pageMap->NumClear()>0);
        // find an idle page on the page map
        int physPage = pageMap->Find();
        DEBUG('u',"phys page: %d allocated to cur addr space: %d\n",physPage,cas->getAsId());
        // fetch the corresponding page of vAddr on the swap disk to this phys page
        logPage = vm2Mem(vAddr,vmpt,physPage*PageSize);
        // user prog's page-in-use inc
        cas->numInUse++;
        // update the pt
        vmpt[logPage].physicalPage = physPage;
        // now the logPage is in the mem
        vmpt[logPage].valid = true;
        printf("Demand page %d in(frame %d)\n",logPage,physPage);
    }else{
        logPage = swapPage(vAddr);
    }
    // remember to add the new ref into the refStk
    refPush(logPage);
    if(true){ //DebugIsEnabled('m')||DebugIsEnabled('a')
        cas->Print();
    }
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if(which == SyscallException){
        if(type == SC_Halt){
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            // never reached
            return;
        }
        else if(type == SC_Exec){
            interrupt->Exec();
        }
        else if(type == SC_PrintInt){
            interrupt->PrintInt();
        }else if(type == SC_Fork){
            interrupt->Fork();
        }else if(type == SC_UExec){
            interrupt->UExec();
        }
        // Note that the PC increases
        IncPc();
        return;
    }
    else if(which == PageFaultException){
        /**
         * Lab7:vmem
         * page fault handle
         */
        //DEBUG('r',"bad vAddr: %d\n",machine->registers[BadVAddrReg]);
        loadPage(machine->registers[BadVAddrReg]);
        stats->numPageFaults++;
        // there's no need to IncPc,since we should restart the instruction
        // just keep the PCs as where they stay now
    }
    else{
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}



