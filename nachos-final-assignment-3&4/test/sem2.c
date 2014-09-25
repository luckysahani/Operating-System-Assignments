#include "syscall.h"
#include "synchop.h"

#define SEMKEY 19
#define SHMKEY 17

int
main()
{
    int id = SemGet(SEMKEY);

    SemOp(id, -1);
    PrintInt(0);
    PrintInt(0);
    PrintInt(0);
    PrintInt(0);
    SemOp(id, 1);
}
