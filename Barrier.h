//
// Created by Administrator on 2023/9/23.
//

#ifndef LAB3_BARRIER_H
#define LAB3_BARRIER_H

#include "copyright.h"
#include "synch.h"

class Barrier {
    public:
        Barrier(const char* debugName,int number);
        ~Barrier();
        void Arrive();
        //void throughCritical();
    private:
        int n;//the number of threads
        int count;
        char* name;
        Condition* allArrive; // whether all the n threads have arrived
        Lock* mutex; //mutex lock to access the count register
};

#endif //LAB3_BARRIER_H
