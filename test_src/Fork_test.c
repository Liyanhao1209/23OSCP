#include "syscall.h"

void func(){
    PrintInt(54321);
}

int
main()
{
    Fork(func);
    PrintInt(12138);
    Halt();
    /* not reached */
}