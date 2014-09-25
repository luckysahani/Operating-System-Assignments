#include "syscall.h"

int
main()
{
    //PrintString("Before calling Exec.\n");
    Exec("../test/vectorsum");
    //PrintString("Returned from Exec.\n"); // Should never return
    return 0;
}
