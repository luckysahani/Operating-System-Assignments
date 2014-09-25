#include "syscall.h"

int
main()
{
    PrintString("Before calling Exec.\n");
    Exec("../test/testregPA");
}

