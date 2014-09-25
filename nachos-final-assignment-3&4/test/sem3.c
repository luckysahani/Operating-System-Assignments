#include "syscall.h"
#include "synchop.h"

#define SEMKEY 19

int
main()
{
    int *array = (int *)ShmAllocate(10*sizeof(int));
    array[0]=1;
    int x = Fork();
    int id = SemGet(SEMKEY);
    if(x == 0) {
        PrintChar('C');
        SemOp(id, -1);
        array[0]=10;
        PrintInt(array[0]);
        Yield();
        SemOp(id, 1);
    } else {
        PrintChar('P');
        while(array[0]!=10);
        SemOp(id, -1);
        array[0]=1;
        PrintInt(array[0]);
        SemOp(id, 1);
    }
}
