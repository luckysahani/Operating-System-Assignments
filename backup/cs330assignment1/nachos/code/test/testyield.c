#include "syscall.h"

int
main()
{
    int x, i;

    x = sys_Fork();
    for (i=0; i<5; i++) {
       sys_PrintString("*** thread ");
       sys_PrintInt(sys_GetPID());
       sys_PrintString(" looped ");
       sys_PrintInt(i);
       sys_PrintString(" times.\n");
       sys_Yield();
    }
    if (x != 0) {
       sys_PrintString("Before join.\n");
       sys_Join(x);
       sys_PrintString("After join.\n");
    }
    return 0;
}
