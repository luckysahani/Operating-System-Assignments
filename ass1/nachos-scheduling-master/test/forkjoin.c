#include "syscall.h"

int
main()
{
    int x;

    PrintString("Parent PID: ");
    PrintInt(GetPID());
    PrintChar('\n');
    x = Fork();
    if (x == 0) {
       PrintString("Child PID: ");
       PrintInt(GetPID());
       PrintChar('\n');
       PrintString("Child's parent PID: ");
       PrintInt(GetPPID());
       PrintChar('\n');
       PrintString("Child calling sleep at time: ");
       PrintInt(GetTime());
       PrintChar('\n');
       //Sleep(100);
       PrintString("Child returned from sleep at time: ");
       PrintInt(GetTime());
       PrintChar('\n');
    }
    else {
       PrintString("Parent after fork waiting for child: ");
       PrintInt(x);
       PrintChar('\n');
       Join(x);
    }
    return 0;
}
