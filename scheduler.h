// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

/**
 * Lab2
 * in case of switching too frequently
*/
#define MinSwitchPace TimerTicks;

enum{Relinquish,Complete,Compelled};
// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();			// Initialize list of ready threads 
    ~Scheduler();			// De-allocate ready list

    void ReadyToRun(Thread* thread);	// Thread can be dispatched.
    Thread* FindNextToRun(int status);		// Dequeue first thread on the ready
					// list, if any, and return thread.
    void Run(Thread* nextThread);	// Cause nextThread to start running
    void Print();			// Print contents of ready list

    /**
     * Lab2/PRIORITY
    */
    void BackToTop(Thread *thread);
  private:
    List *readyList;  		// queue of threads that are ready to run,but not running

#ifdef AGING
  private:
    /**
     * Lab2/Aging
     * maintain the last time of thread switching,to adjust the priority
    */
    int lastSwitchTick; 
  public:
    /**
     * Lab2/Aging
     * flush the priority of all the ready threads
    */
    void FlushPriority();
    /**
     * Lab2/Aging
     * getter/setter of lastSwitchTick
    */
    int getLastSwitchTick();
    void setLastSwitchTick(int newSwitchTick);
#endif
};

#endif // SCHEDULER_H
