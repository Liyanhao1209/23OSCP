// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"
#include "interrupt.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List;
#ifdef AGING
    lastSwitchTick = 0;
#endif
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    //DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
    //readyList->Append((void *)thread);
    /**
     * Lab2/PRIORITY
     * when a thread has static priority,we should find a proper location in the linear list for it by checking its priority
    */
    #ifdef PRIORITY
    readyList->SortedInsert((void *)thread,thread->getPriority());
    #endif

    #ifdef ROUNDROBIN
    /**
     * Lab2/ROUNDROBIN
     * if the time slice is not expired,go to the head of the list
     * otherwise,the end of the list
    */
    readyList->Append((void*)thread);
    #endif

    #ifdef FCFS
    readyList->Append((void*)thread);
    #endif

}

/**
 * Lab2/PRIORITY
 * when we are not in a priority mode
 * we pick a thread from the top of the list
 * in case that a switch occur
 * however,if the switch did not happen
 * we send the thread back to the top of the list
*/
void
Scheduler::BackToTop(Thread *thread){
    thread->setStatus(READY);
    readyList->Prepend((void*)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

void PrintReadyList(Thread* t){
    printf("t name: %s\n",t->getName());
}

Thread *
Scheduler::FindNextToRun (int status)
{
    if(DebugIsEnabled('t')){
        readyList->Mapcar((VoidFunctionPtr) PrintReadyList);
    }
    Thread* nextThread = (Thread *)readyList->Remove();
    // current thread is voluntary to relinquish the cpu to others,
    // it's time for next thread to run(if there still has any)
    if(status != Compelled){
        /**
         * Lab2/TIMESLICE
         * When a thread volunteered to relinquish the cpu
         * We should ensure that the time slice is reallocated
         * otherwise,the next thread would have the rest of the time slice prepared for the dying thread!
         */
#ifdef TIMESLICE
        DEBUG('t',"current sys tick = %d\n",stats->systemTicks);
        interrupt->nextTimeSlice()->when = stats-> systemTicks + timer->TimeOfNextInterrupt() ;
        interrupt->getPendingList()->top()->setKey(interrupt->nextTimeSlice()->when);
        DEBUG('t',"next time slice =  %d\n",interrupt->nextTimeSlice()->when);
#endif
        if(status== Relinquish){
            scheduler->ReadyToRun(currentThread);
        }
        return nextThread;
    }
    /**
     * Lab2
     * we initially suppose the current thread would be the loser
     * when compete with the one we pick from the ready queue
    */
    Thread* loser = currentThread;
    if(nextThread!=NULL){
        /**
         * Lab2
         * when we are scheduling the threads with priority(no matter static or dynamic ones)
         * we should compare the one we select from the ready queue with current thread
         * and pick one with higher priority to execute while the other should be put back into ready queue
        */
        bool priorityCmp = false;
#ifdef PRIORITY
        //lower number means higher priority
        priorityCmp = currentThread->getPriority()<nextThread->getPriority();
#endif
         // necessary condition for switching
        // the new thread's priority is higher than the currentThread
        if(priorityCmp){
            loser = nextThread;
            nextThread = currentThread;
        }
#ifdef PRIORITY
        scheduler->ReadyToRun(loser);
#else
        scheduler->BackToTop(loser)
#endif
    }
    return nextThread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    
    /**
     * Lab2/AGING
     * update the lastSwitchTick before we switch the thread
    */
    #ifdef AGING
    lastSwitchTick = stats->systemTicks;
    #endif

    SWITCH(oldThread, nextThread);
    
    //DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

#ifdef AGING
/**
 * Lab2/Aging
 * increase the priority of a specific thread
*/
void
increPriority(Thread* thread){
    int changePri;
    changePri = thread->getPriority()+AGING_PACE;
    if (changePri<0)
    {
        changePri=0;
    }
    thread->setPriority(changePri);
}

/**
 * Lab2/Aging
 * flush the priority of all the ready threads
*/
void
Scheduler::FlushPriority(){
    readyList->Mapcar((VoidFunctionPtr)increPriority);
}

/**
 * Lab2/Aging
 * getter/setter of lastSwitchTick
*/
int
Scheduler::getLastSwitchTick()
{
    return this->lastSwitchTick;
}

void
Scheduler::setLastSwitchTick(int newSwitchTick){
    this->lastSwitchTick = newSwitchTick;
}
#endif