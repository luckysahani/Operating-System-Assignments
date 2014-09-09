#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;

    for (i=0; i<SIZE; i++) array[i] = i;
    for (i=0; i<SIZE; i++) sum += array[i];
    sys_PrintString("Total sum: ");
    sys_PrintInt(sum);
    sys_PrintChar('\n');
    sys_PrintString("Executed instruction count: ");
    sys_PrintInt(sys_GetNumInstr());
    sys_PrintChar('\n');
    sys_Exit(0);
    return 0;
}
