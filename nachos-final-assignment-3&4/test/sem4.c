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
        SemOp(id, -1);
        PrintChar('C');
        array[0]=10;
        SemOp(id, 1);
    } else {
        Join(x);
        SemOp(id, -1);
        PrintChar('P');
        PrintInt(array[0]);
        SemOp(id, 1);
    }
}
