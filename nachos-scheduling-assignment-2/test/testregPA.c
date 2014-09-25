#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;

    PrintString("Starting physical address of array: ");
    PrintInt(GetPA((unsigned)array));
    PrintChar('\n');
    PrintString("Physical address of array[50]: ");
    PrintInt(GetPA((unsigned)&array[50]));
    PrintChar('\n');
    PrintString("Current physical address of stack top: ");
    PrintInt(GetPA(GetReg(29)));
    PrintChar('\n');
    for (i=0; i<SIZE; i++) array[i] = i;
    PrintString("We are currently at PC: ");
    PrintIntHex(GetReg(34));
    PrintChar('\n');
    for (i=0; i<SIZE; i++) sum += array[i];
    PrintString("Total sum: ");
    PrintInt(sum);
    PrintChar('\n');
    Exit(0);
    return 0;
}
