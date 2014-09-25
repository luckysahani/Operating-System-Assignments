#include "syscall.h"
#include "synchop.h"

#define SEM_KEY1 19
#define SEM_KEY2 20
#define COND_KEY1 19
#define COND_KEY2 20
#define SIZE 10
#define NUM_ENQUEUER 4
#define NUM_DEQUEUER 1
#define DEQUEUE_EXIT_CODE 20
#define ENQUEUE_EXIT_CODE 10
#define NUM_ENQUEUE_OP 10
#define NUM_DEQUEUE_OP ((NUM_ENQUEUE_OP*NUM_ENQUEUER)/NUM_DEQUEUER)

int *array, semid, stdoutsemid, notFullid, notEmptyid;

int
Enqueue (int x, int id)
{
   int y;

   SemOp(semid, -1);
   while (array[SIZE+2] == SIZE) {
      SemOp(stdoutsemid, -1);
      PrintString("Enqueuer ");
      PrintInt(id);
      PrintString(": waiting on queue full.");
      PrintChar('\n');
      SemOp(stdoutsemid, 1);
      CondOp(notFullid, COND_OP_WAIT, semid);
   }
   array[array[SIZE+1]] = x;
   y = array[SIZE+1];
   array[SIZE+1] = (array[SIZE+1] + 1)%SIZE;
   array[SIZE+2]++;
   CondOp(notEmptyid, COND_OP_SIGNAL, semid);
   SemOp(semid, 1);
   return y;
}

int
Dequeue (int id, int *y)
{
   int x;

   SemOp(semid, -1);
   while (array[SIZE+2] == 0) {
      SemOp(stdoutsemid, -1);
      PrintString("Dequeuer ");
      PrintInt(id);
      PrintString(": waiting on queue empty.");
      PrintChar('\n');
      SemOp(stdoutsemid, 1);
      CondOp(notEmptyid, COND_OP_WAIT, semid);
   }
   x = array[array[SIZE]];
   (*y) = array[SIZE];
   array[SIZE] = (array[SIZE] + 1)%SIZE;
   array[SIZE+2]--;
   CondOp(notFullid, COND_OP_SIGNAL, semid);
   SemOp(semid, 1);
   return x;
}

int
main()
{
    array = (int*)ShmAllocate(sizeof(int)*(SIZE+3));  // queue[SIZE], head, tail, count
    int x, i, j, seminit = 1, y;
    int pid[NUM_DEQUEUER+NUM_ENQUEUER];

    for (i=0; i<SIZE; i++) array[i] = -1;
    array[SIZE] = 0;
    array[SIZE+1] = 0;
    array[SIZE+2] = 0;

    semid = SemGet(SEM_KEY1);
    SemCtl(semid, SYNCH_SET, &seminit);

    stdoutsemid = SemGet(SEM_KEY2);
    SemCtl(stdoutsemid, SYNCH_SET, &seminit);

    notFullid = CondGet(COND_KEY1);
    notEmptyid = CondGet(COND_KEY2);

    for (i=0; i<NUM_DEQUEUER; i++) {
       x = Fork();
       if (x == 0) {
          for (j=0; j<NUM_DEQUEUE_OP; j++) {
             x = Dequeue (i, &y);
             SemOp(stdoutsemid, -1);
             PrintString("Dequeuer ");
             PrintInt(i);
             PrintString(": Got ");
             PrintInt(x);
             PrintString(" from slot ");
             PrintInt(y);
             PrintChar('\n');
             SemOp(stdoutsemid, 1);
          }
          Exit(DEQUEUE_EXIT_CODE);
       }
       pid[i] = x;
    }
    
    for (i=0; i<NUM_ENQUEUER; i++) {
       x = Fork();
       if (x == 0) {
          x = i*NUM_ENQUEUE_OP;
          for (j=0; j<NUM_ENQUEUE_OP; j++) {
             y = Enqueue (x+j, i);
             SemOp(stdoutsemid, -1);
             PrintString("Enqueuer ");
             PrintInt(i);
             PrintString(": Inserted ");
             PrintInt(x+j);
             PrintString(" in slot ");
             PrintInt(y);
             PrintChar('\n');
             SemOp(stdoutsemid, 1);
          }
          Exit(ENQUEUE_EXIT_CODE);
       }
       pid[i+NUM_DEQUEUER] = x;
    }

    for (i=0; i<NUM_DEQUEUER+NUM_ENQUEUER; i++) {
       x = Join(pid[i]);
       SemOp(stdoutsemid, -1);
       PrintString("Parent joined with ");
       PrintInt(pid[i]);
       PrintString(" having exit code ");
       PrintInt(x);
       PrintChar('\n');
       SemOp(stdoutsemid, 1);
    }
    SemCtl(semid, SYNCH_REMOVE, 0);
    SemCtl(stdoutsemid, SYNCH_REMOVE, 0);
    CondRemove(notFullid);
    CondRemove(notEmptyid);

    return 0;
}
