#include "syscall.h"

int
main()
{
    SpaceId pid;
    PrintInt(54321);
    pid = Exec("../test/exec.noff");
    Halt();
    /* not reached */
}
