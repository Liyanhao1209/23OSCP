#include "replace.h"

void refPush(int* ref){
    currentThread->space->refStk->Append((void*)ref);
}

int* refPop(){
    return (int*)currentThread->space->refStk->Remove();
}

int calVictim(){
    // step 1: get the user prog ref stack
    List* rs = currentThread->space->refStk;

    // step 2: pop the ele at the stk bottom
    int* ret = refPop();

    return *ret;
}