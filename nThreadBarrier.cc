#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "copyright.h"
#include "system.h"
#include "synch.h"

#define N_THREADS  10    // the number of threads
#define N_TICKS    1000  // the number of ticks to advance simulated time
#define MAX_NAME  16 // max name length

class Barrier {
public:
    Barrier(const char* debugName,int number);
    ~Barrier();
    void Arrive(int index);
private:
    int n;//the number of threads
    int count;
    char* name;
    Semaphore* turnstile;
    Lock* mutex; //mutex lock to access the count register
};

Barrier::Barrier(const char* debugName,int number) {
    name = (char*)debugName;
    n = number;
    turnstile = new Semaphore(name,0);
    mutex = new Lock(name);
    count=0;
}

Barrier::~Barrier() {
    delete turnstile;
    delete mutex;
}

/**
 * Lab3/Barrier
 * current thread arrive at the turnstile
 * check if it is the very last one
 * if so,signal the threads blocked before one by one
 */
void Barrier::Arrive(int index) {
    // revert the counter
    if(count>=n){
        count=0;
    }


    mutex->Acquire();
    count = count +1;
    printf("Thread [%d] has arrived\n",index);
    if(count==n){
        turnstile->V();
        printf("Thread [%d] is the last\n", index);
    }
    mutex->Release();

    turnstile->P();
    turnstile->V();

    //simulate critical point
    printf("Thread [%d] cross through Critical Section\n",index);
}

// global variables to realize N thread barrier test
Barrier* barrier;
Thread* threads[N_THREADS];
char threads_name[N_THREADS][MAX_NAME];
Semaphore *advance;

void MakeTicks(int n)  // advance n ticks of simulated time
{
    //use the IntOff and IntOn to advance the clock
    advance = new Semaphore("advance",1);
    for(int i=0;i<n;i++){
        advance->P();
        advance->V();
    }
}


void BarThread(_int which)
{
    MakeTicks(N_TICKS);
    printf("Thread [%d] rendezvous\n", which);
    barrier->Arrive(which);
}


void ThreadsBarrier()
{
    barrier = new Barrier("N_thread",N_THREADS);
    // Create and fork N_THREADS threads
    for(int i = 0; i < N_THREADS; i++) {
        sprintf(threads_name[i], "Thread %d", i);
        threads[i] = new Thread(threads_name[i]);
        threads[i]->Fork(BarThread, i);
    };
}

