#include "syscall.h"

int
main()
{
    int x, i;

    x = Fork();
    for (i=0; i<5; i++) {
       PrintString("*** thread ");
       PrintInt(GetPID());
       PrintString(" looped ");
       PrintInt(i);
       PrintString(" times.\n");
       Yield();
    }
    if (x != 0) {
       PrintString("Before join.\n");
       Join(x);
       PrintString("After join.\n");
    }
    return 0;
}
