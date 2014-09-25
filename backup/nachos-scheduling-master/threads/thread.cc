// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

// Stativ variable to keep a track of the total number of threads
int Thread::threadCount = 0;

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName)
{
    int i;

    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif

    // The three variables defined below are to compute the CPU 
    // utilization and other parameters of the process
    start_time = 0;
    total_time = 0;
    cpu_time = 0;
    cpu_burst_start = 0;
    cpu_burst_previous = 0.0; 
    cpu_burst_estimate = 200.0;
    wait_time = 0;
    wait_time_start = 0;
    block_time = 0;
    block_start = 0;
    timerYield = false;

    threadArray[thread_index] = this;
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if (currentThread != NULL) {
       ppid = currentThread->GetPID();
       currentThread->RegisterNewChild (pid);
    }
    else ppid = -1;

    childcount = 0;
    waitchild_id = -1;

    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    base_priority = 50;
    priority = base_priority;
    DEBUG('s', "Creating \"%d\" base priority %d\n",pid, base_priority);

    // Increment the threadCount
    threadCount++;
}

//----------------------------------------------------------------------
// Thread::Thread
//	"orphan" sets the ppid to -1 if true
// same as Thread but sets the priority equal to the given value
//----------------------------------------------------------------------

Thread::Thread(char* threadName, int newPriority, bool orphan)
{
    int i;

    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif

    // The three variables defined below are to compute the CPU 
    // utilization and other parameters of the process
    start_time = 0;
    total_time = 0;
    cpu_time = 0;
    cpu_burst_start = 0;
    cpu_burst_previous = 0.0; 
    cpu_burst_estimate = 200.0;
    wait_time = 0;
    wait_time_start = 0;
    block_time = 0;
    block_start = 0;
    timerYield = false;

    threadArray[thread_index] = this;
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if(orphan == true) {
        ppid = -1;
    } else {
        if (currentThread != NULL) {
           ppid = currentThread->GetPID();
           currentThread->RegisterNewChild (pid);
        }
        else ppid = -1;
    }

    childcount = 0;
    waitchild_id = -1;

    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    base_priority = 50 + newPriority; //set priority of the thread
    priority = base_priority;
    DEBUG('s', "Creating \"%d\" base priority %d\n",pid, base_priority);

    // Increment the threadCount
    threadCount++;
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%d\"\n", pid);

    ASSERT(this != currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));

    // Decrement the threadCount
    threadCount--;
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, int arg)
{
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, arg);
    
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that Scheduler::Run() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%d\"\n", GetPID());
    
    threadToBeDestroyed = currentThread;
    Sleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::SetChildExitCode
//      Called by an exiting thread on parent's thread object.
//----------------------------------------------------------------------

void
Thread::SetChildExitCode (int childpid, int ecode)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   ASSERT(i < childcount);
   childexitcode[i] = ecode;
   exitedChild[i] = true;

   if (waitchild_id == (int)i) {
      waitchild_id = -1;
      // I will wake myself up
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      scheduler->ReadyToRun(this);
      (void) interrupt->SetLevel(oldLevel);
   }
}

//----------------------------------------------------------------------
// Thread::Exit
//      Called by ExceptionHandler when a thread calls Exit.
//      The argument specifies if all threads have called Exit, in which
//      case, the simulation should be terminated.
//----------------------------------------------------------------------

void
Thread::Exit (bool terminateSim, int exitcode)
{
    (void) interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%d\"\n", GetPID());
    threadToBeDestroyed = currentThread;

    Thread *nextThread;

    // Update the information in the threadArray
    threadArray[currentThread->GetPID()] = NULL;

    // BURST_STAT_UPDATE
    // Update the information about the currentThread, with is the currentThread right now
    currentThread->cpu_burst_previous = stats->totalTicks - currentThread->cpu_burst_start;
    currentThread->cpu_time += currentThread->cpu_burst_previous;

    DEBUG('s', "\n[ pid %d ] start time %d, current time %d, CPU burst time %d\n", 
            currentThread->GetPID(), currentThread->cpu_burst_start, 
            stats->totalTicks, currentThread->cpu_burst_previous);

    if(currentThread->GetPID() > 0) {
        if(currentThread->cpu_burst_previous > 0) { 
            // For counting the number of burst
            DEBUG('S', "\nBURST_STAT_UPDATE");
            stats->total_cpu += currentThread->cpu_burst_previous;
            stats->burst_count++;

            // Check for maximum
            if(currentThread->cpu_burst_previous > stats->max_cpu) {
                stats->max_cpu = currentThread->cpu_burst_previous;
            }

            //Check for minimum
            if(currentThread->cpu_burst_previous < stats->min_cpu) {
                stats->min_cpu = currentThread->cpu_burst_previous;
            }
        }
    }

    // UNIX_SCHEDULING
    // Increment the priorities of the threads
    if(currentThread->cpu_burst_previous > 0) {
        if (scheduler->scheduler_type >= 7 && scheduler->scheduler_type <= 10) {
            int i, pid = currentThread->GetPID();

            DEBUG('U', "\nUpdate initiated by %d time %d burst %d\n",
                    currentThread->GetPID(), stats->totalTicks,
                    currentThread->cpu_burst_previous);

            // Update the cpu_count
            scheduler->cpu_count[pid] += currentThread->cpu_burst_previous;

            // Half all the cpu_counts and update the priorities of all threads
            for(i=0; i<MAX_THREAD_COUNT; ++i) {
                scheduler->cpu_count[i] = scheduler->cpu_count[i]/2;
                if(threadArray[i] != NULL) {
                    threadArray[i]->priority += scheduler->cpu_count[i]/2;
                    DEBUG('U', "Thread %i Priority %d\n", i, threadArray[i]->priority);
                }
            }
        }
    }

    status = BLOCKED;

    // Note the time for which the thread is blocked
    currentThread->block_start = stats->totalTicks;

    // Set exit code in parent's structure provided the parent hasn't exited
    if (ppid != -1) {
       ASSERT(threadArray[ppid] != NULL);
       if (!exitThreadArray[ppid]) {
          threadArray[ppid]->SetChildExitCode (pid, exitcode);
       }
    }

    // If there are no threads left to execute then stop the machine
    if(threadCount == 1) {
        terminateSim = true;
    }
    
    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
        if (terminateSim) {
            DEBUG('i', "Machine idle.  No interrupts to do.\n");
            printf("\nNo threads ready or runnable, and no pending interrupts.\n");
            printf("Assuming all programs completed.\n");
            interrupt->Halt();
        }
        else interrupt->Idle();      // no one to run, wait for an interrupt
    }

    // Printing the statistics of this thread as this is about to exit
    currentThread->block_time += stats->totalTicks - currentThread->block_start;
    currentThread->total_time += stats->totalTicks - currentThread->start_time;
    DEBUG('s' , "\nThread %d total %d cpu %d wait %d block %d\n", 
            currentThread->GetPID(),
            currentThread->total_time,
            currentThread->cpu_time, currentThread->wait_time, 
            currentThread->block_time);

    // FINAL_STAT_UPDATE
    // Update the thread statistics
    if(currentThread->GetPID() > 0 ){
        DEBUG('s', "\nFINAL_STAT_UPDATE");
        stats->thread_count++;
        stats->total_wait += currentThread->wait_time;
        stats->total_thread += currentThread->total_time;
        stats->square_thread += currentThread->total_time * currentThread->total_time;

        int completion_time = stats->totalTicks;
        stats->total_completion += completion_time;
        stats->square_completion += (long long int)completion_time * (long long int)completion_time;
        DEBUG('C', "Thread %d Completion time %d square Completion %lld\n", 
                currentThread->GetPID(), completion_time, 
                stats->square_completion);
        
        // maxmimum value of thread completion
        if(stats->max_thread < currentThread->total_time) {
            stats->max_thread = currentThread->total_time;
        }

        // minumum value of thread completion
        if(currentThread->total_time < stats->min_thread ) {
            stats->min_thread = currentThread->total_time;
        }

        // The maximum and minimum completion
        if(completion_time > stats->max_completion) {
            stats->max_completion = completion_time;
        }

        if(completion_time < stats->min_completion) {
            stats->min_completion = completion_time;
        }
    }

    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
    DEBUG('t', "Yielding thread \"%d\"\n", pid);

    // BURST_STAT_UPDATE
    // Update the information about the currentThread, with is the currentThread right now
    currentThread->cpu_burst_previous = stats->totalTicks - currentThread->cpu_burst_start;
    currentThread->cpu_time += currentThread->cpu_burst_previous;

    DEBUG('s', "\n[ pid %d ] start time %d, current time %d, CPU burst time %d\n", 
            currentThread->GetPID(), currentThread->cpu_burst_start, 
            stats->totalTicks, currentThread->cpu_burst_previous);

    if(currentThread->GetPID() > 0) {
        if(currentThread->cpu_burst_previous > 0) { 
            DEBUG('s', "BURST_STAT_UPDATE");
            // For counting the number of burst
            stats->total_cpu += currentThread->cpu_burst_previous;
            stats->burst_count++;

            // Check for maximum
            if(currentThread->cpu_burst_previous > stats->max_cpu) {
                stats->max_cpu = currentThread->cpu_burst_previous;
            }

            //Check for minimum
            if(currentThread->cpu_burst_previous < stats->min_cpu) {
                stats->min_cpu = currentThread->cpu_burst_previous;
            }
        }
    }

    // UNIX_SCHEDULING
    // increment the priority of all threads
    if(currentThread->cpu_burst_previous > 0) {
        if (scheduler->scheduler_type >= 7 && scheduler->scheduler_type <= 10) {
            int i, pid = currentThread->GetPID();

            DEBUG('U', "\nUpdate initiated by %d time %d burst %d\n",
                    currentThread->GetPID(), stats->totalTicks,
                    currentThread->cpu_burst_previous);

            // Update the cpu_count
            scheduler->cpu_count[pid] += currentThread->cpu_burst_previous;

            // Half all the cpu_counts and update the priorities of all threads
            for(i=0; i<MAX_THREAD_COUNT; ++i) {
                scheduler->cpu_count[i] = scheduler->cpu_count[i]/2;
                if(threadArray[i] != NULL) {
                    threadArray[i]->priority += scheduler->cpu_count[i]/2;
                    DEBUG('U', "Thread %i Priority %d\n", i, threadArray[i]->priority);
                }
            }
        }
    }

    // If it is a timerYield then we add this thread to the ready list first and
    // then compute the next thread
    if(timerYield) {
        scheduler->ReadyToRun(this);
    }
    nextThread = scheduler->FindNextToRun();

    if (nextThread != NULL) {
        if(timerYield) {
            timerYield = false;
        } else {
            scheduler->ReadyToRun(this);
        }
        scheduler->Run(nextThread);
    }
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%d\"\n", GetPID());

    // BURST_STAT_UPDATE
    // Update the information about the currentThread, with is the currentThread right now
    currentThread->cpu_burst_previous = stats->totalTicks - currentThread->cpu_burst_start;
    currentThread->cpu_time += currentThread->cpu_burst_previous;

    DEBUG('s', "\n[ pid %d ] start time %d, current time %d, CPU burst time %d\n", 
            currentThread->GetPID(), currentThread->cpu_burst_start, 
            stats->totalTicks, currentThread->cpu_burst_previous);

    if(currentThread->GetPID() > 0) {
        if(currentThread->cpu_burst_previous > 0) { 
            DEBUG('s', "BURST_STAT_UPDATE");
            // For counting the number of burst
            stats->total_cpu += currentThread->cpu_burst_previous;
            stats->burst_count++;

            // Check for maximum
            if(currentThread->cpu_burst_previous > stats->max_cpu) {
                stats->max_cpu = currentThread->cpu_burst_previous;
            }

            //Check for minimum
            if(currentThread->cpu_burst_previous < stats->min_cpu) {
                stats->min_cpu = currentThread->cpu_burst_previous;
            }
        }
    }

    // UNIX_SCHEDULING
    // increment the priority of all threads
    if(currentThread->cpu_burst_previous > 0) {
        if (scheduler->scheduler_type >= 7 && scheduler->scheduler_type <= 10) {
            int i, pid = currentThread->GetPID();

            DEBUG('U', "\nUpdate initiated by %d time %d burst %d\n",
                    currentThread->GetPID(), stats->totalTicks,
                    currentThread->cpu_burst_previous);

            // Update the cpu_count
            scheduler->cpu_count[pid] += currentThread->cpu_burst_previous;

            // Half all the cpu_counts and update the priorities of all threads
            for(i=0; i<MAX_THREAD_COUNT; ++i) {
                scheduler->cpu_count[i] = scheduler->cpu_count[i]/2;
                if(threadArray[i] != NULL) {
                    threadArray[i]->priority += scheduler->cpu_count[i]/2;
                    DEBUG('U', "Thread %i Priority %d\n", i, threadArray[i]->priority);
                }
            }
        }
    }

    status = BLOCKED;

    // To compute the block time
    currentThread->block_start = stats->totalTicks;
    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
        interrupt->Idle();	// no one to run, wait for an interrupt
    }

    currentThread->block_time += stats->totalTicks - currentThread->block_start;
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)_ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (int) _ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
}

//----------------------------------------------------------------------
// Thread::CheckIfChild
//      Checks if the passed pid belongs to a child of mine.
//      Returns child id if all is fine; otherwise returns -1.
//----------------------------------------------------------------------

int
Thread::CheckIfChild (int childpid)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   if (i == childcount) return -1;
   return i;
}

//----------------------------------------------------------------------
// Thread::JoinWithChild
//      Called by a thread as a result of SC_Join.
//      Returns the exit code of the child being joined with.
//----------------------------------------------------------------------

int
Thread::JoinWithChild (int whichchild)
{
   // Has the child exited?
   if (!exitedChild[whichchild]) {
      // Put myself to sleep
      waitchild_id = whichchild;
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      printf("[pid %d] Before sleep in JoinWithChild.\n", pid);
      Sleep();
      printf("[pid %d] After sleep in JoinWithChild.\n", pid);
      (void) interrupt->SetLevel(oldLevel);
   }
   return childexitcode[whichchild];
}

//----------------------------------------------------------------------
// Thread::ResetReturnValue
//      Sets the syscall return value to zero. Used to set the return
//      value of SC_Fork in the created child.
//----------------------------------------------------------------------

void
Thread::ResetReturnValue ()
{
   userRegisters[2] = 0;
}

//----------------------------------------------------------------------
// Thread::Schedule
//      Enqueues the thread in the ready queue.
//----------------------------------------------------------------------

void
Thread::Schedule()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);        // ReadyToRun assumes that interrupts
                                        // are disabled!
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Startup
//      Part of the scheduling code needed to cleanly start a forked child.
//----------------------------------------------------------------------

void
Thread::Startup()
{
   scheduler->Tail();
}

//----------------------------------------------------------------------
// Thread::SortedInsertInWaitQueue
//      Called by SC_Sleep before putting the caller thread to sleep
//----------------------------------------------------------------------

void
Thread::SortedInsertInWaitQueue (unsigned when)
{
   TimeSortedWaitQueue *ptr, *prev, *temp;

   if (sleepQueueHead == NULL) {
      sleepQueueHead = new TimeSortedWaitQueue (this, when);
      ASSERT(sleepQueueHead != NULL);
   }
   else {
      ptr = sleepQueueHead;
      prev = NULL;
      while ((ptr != NULL) && (ptr->GetWhen() <= when)) {
         prev = ptr;
         ptr = ptr->GetNext();
      }
      if (ptr == NULL) {  // Insert at tail
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ASSERT(prev->GetNext() == NULL);
         prev->SetNext(ptr);
      }
      else if (prev == NULL) {  // Insert at head
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ptr->SetNext(sleepQueueHead);
         sleepQueueHead = ptr;
      }
      else {
         temp = new TimeSortedWaitQueue (this, when);
         ASSERT(temp != NULL);
         temp->SetNext(ptr);
         prev->SetNext(temp);
      }
   }

   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   //printf("[pid %d] Going to sleep at %d.\n", pid, stats->totalTicks);
   Sleep();
   //printf("[pid %d] Returned from sleep at %d.\n", pid, stats->totalTicks);
   (void) interrupt->SetLevel(oldLevel);
}
#endif
