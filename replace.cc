#include "replace.h"

void refPush(int ref){
    DEBUG('r',"new ref: %d\n",ref);
    AddrSpace* as = currentThread->space;
    // still enough space
    as->refStk[as->stkSize++] = ref;
}

int refPop(){
    AddrSpace* as = currentThread->space;
    int ret = as->refStk[0];
    // Each element moves forward
    for(int i=1;i<as->stkSize;i++){
        as->refStk[i-1] = as->refStk[i];
    }
    as->stkSize--;
    return ret;
}

int calVictim(){
    return refPop();
}