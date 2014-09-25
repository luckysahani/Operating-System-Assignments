#include "syscall.h"

int
main()
{
    sys_PrintString("Before calling Exec.\n");
    sys_Exec("../test/vectorsum");
    sys_PrintString("Returned from Exec.\n"); // Should never return
    return 0;
}
