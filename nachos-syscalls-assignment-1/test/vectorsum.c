#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;

    for (i=0; i<SIZE; i++) array[i] = i;
    for (i=0; i<SIZE; i++) sum += array[i];
    PrintString("Total sum: ");
    PrintInt(sum);
    PrintChar('\n');
    Exit(0);
    return 0;
}
