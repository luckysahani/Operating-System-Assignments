#include "syscall.h"

int
main()
{
    int *array = (int *)ShmAllocate(200*sizeof(int));
    int x = Fork();
    if (x == 0) {
        PrintChar('\n');
        PrintInt(GetPA((unsigned)&array[0]));
        Exit(0);
    } else {
        Join(x);
        PrintInt(GetPA((unsigned)&array[0]));
        PrintChar('\n');
    }
    return 0;
}
