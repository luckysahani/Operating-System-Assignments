#include "syscall.h"
#include "synchop.h"

#define SEMKEY 19

int
main()
{
    int id = SemGet(SEMKEY);

    SemOp(id, -1);
    PrintInt(10);
    SemOp(id, 1);

}
