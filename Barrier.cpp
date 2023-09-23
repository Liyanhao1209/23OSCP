#include "copyright.h"
#include "Barrier.h"
#include <string>

#define N_THREADS  10    // the number of threads
#define N_TICKS    1000  // the number of ticks to advance simulated time

Barrier::Barrier(const char* debugName,int number) {
    name = (char*)debugName;
    n = number;
    allArrive = new Condition(debugName);
    mutex = new Lock(debugName);
    count=0;
}

Barrier::~Barrier() {
    delete allArrive;
    delete mutex;
}

/**
 * Lab3/Barrier
 * current thread arrive at the turnstile
 * check if it is the very last one
 * if so,signal the threads blocked before one by one
 */
void Barrier::Arrive() {
    // revert the counter
    if(count>=n){
        count=0;
    }
    mutex->Acquire();
    count +=1;
    if(n==count){
        allArrive->Signal();
    }
    mutex->Release();

    allArrive->Wait();
    allArrive->Signal();
}


// global var to realize N thread barrier
Barrier* barrier;
Thread* threads[N_THREADS]
Semaphore *advance;

void MakeTicks(int n)  // advance n ticks of simulated time
{
    //use the IntOff and IntOn to advance the clock
    advance = new Semaphore("advance",1);
    for(int i=n;i>0;i--){
        advance->P();
        advance->V();
    }
}


void BarThread(_int which)
{
    MakeTicks(N_TICKS);
    printf("Thread %d rendezvous\n", which);
    barrier->Arrive();
    printf("Thread %d critical point\n", which);
}


void ThreadsBarrier()
{
    barrier = new Barrier("N thread",N_THREADS);
    // Create and fork N_THREADS threads
    for(i = 0; i < N_THREADS; i++) {
        string s = "Thread"+std::to_string(i);
        threads[i] = new Thread(s);
        threads[i]->Fork(BarThread, i);
    };
}
