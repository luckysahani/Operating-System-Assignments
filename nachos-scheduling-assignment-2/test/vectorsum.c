#include "syscall.h"
#define SIZE 100
int
main()
{
    int array[SIZE], i, sum=0;
    /*unsigned start_time, end_time;*/
    
    /*start_time = GetTime();*/
    for (i=0; i<SIZE; i++) array[i] = i;
    for (i=0; i<SIZE; i++) sum += array[i];
    /*end_time = GetTime();*/
    PrintString("Total sum: ");
    PrintInt(sum);
    PrintChar('\n');
    /*PrintString("Start time: ");
    PrintInt(start_time);
    PrintString(", End time: ");
    PrintInt(end_time);
    PrintString(", Total time: ");
    PrintInt(end_time-start_time);
    PrintChar('\n');*/
    Exit(0);
    return 0;
}
