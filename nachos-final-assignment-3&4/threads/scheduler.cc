// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List;
    empty_ready_queue_start_time = -1;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    if (thread->getStatus() == RUNNING) {
       stats->cpu_time += (stats->totalTicks - cpu_burst_start_time);
       if ((stats->totalTicks - cpu_burst_start_time) > 0) {
          stats->cpu_burst_count++;
          stats->preemptive_switch++;
          if ((stats->totalTicks - cpu_burst_start_time) > stats->max_cpu_burst) {
             stats->max_cpu_burst = (stats->totalTicks - cpu_burst_start_time);
          }
          if ((stats->totalTicks - cpu_burst_start_time) < stats->min_cpu_burst) {
             stats->min_cpu_burst = (stats->totalTicks - cpu_burst_start_time);
          }
          if (schedulingAlgo == UNIX_SCHED) {
             UpdateThreadPriority();
          }
          else if (schedulingAlgo == NON_PREEMPTIVE_SJF) {
             stats->burstEstimateError += abs(stats->totalTicks - cpu_burst_start_time - thread->GetPriority());
             thread->SetPriority((int)(ALPHA*(stats->totalTicks - cpu_burst_start_time) + (1-ALPHA)*thread->GetPriority()));
          }
       }
    }
    thread->setStatus(READY);
    thread->SetWaitStartTime(stats->totalTicks);
    if (readyList->IsEmpty() && (empty_ready_queue_start_time != -1)) {
       stats->empty_ready_queue_time += (stats->totalTicks - empty_ready_queue_start_time);
       empty_ready_queue_start_time = -1;
    }
    readyList->Append((void *)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    if ((schedulingAlgo == UNIX_SCHED) || (schedulingAlgo == NON_PREEMPTIVE_SJF)){
       return (Thread *)readyList->GetMinPriorityThread();
    }
    else {
       return (Thread *)readyList->Remove();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    
    cpu_burst_start_time = stats->totalTicks;
    nextThread->SetCPUBurstStartTime(cpu_burst_start_time);
    stats->total_wait_time += (stats->totalTicks - nextThread->GetWaitStartTime());

#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    _SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Tail
//      This is the portion of Scheduler::Run after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
Scheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}

void
Scheduler::SetEmptyReadyQueueStartTime (int ticks)
{
   empty_ready_queue_start_time = ticks;
}

//-------------------------------------------------------------------------
// Scheduler::UpdateThreadPriority
//      Updates the priority of all active threads as in the UNIX scheduler
//--------------------------------------------------------------------------
void
Scheduler::UpdateThreadPriority (void)
{
   unsigned i;
   int this_cpu_burst_duration = stats->totalTicks - cpu_burst_start_time;
   ASSERT(this_cpu_burst_duration > 0);
   unsigned currentPID = currentThread->GetPID();

   // First we update the currentThread priority

   int currentThreadUsage = currentThread->GetUsage();
   currentThreadUsage = (currentThreadUsage + this_cpu_burst_duration) >> 1;
   int currentThreadPriority = currentThread->GetBasePriority() + (currentThreadUsage >> 1);
   currentThread->SetUsage(currentThreadUsage);
   currentThread->SetPriority(currentThreadPriority);

   // Update everybody else

   for (i=0; i<thread_index; i++) {
      if ((i != currentPID) && !exitThreadArray[i]) {
         ASSERT(threadArray[i] != NULL);
         currentThreadUsage = threadArray[i]->GetUsage();
         currentThreadUsage = currentThreadUsage >> 1;
         currentThreadPriority = threadArray[i]->GetBasePriority() + (currentThreadUsage >> 1);
         threadArray[i]->SetUsage(currentThreadUsage);
         threadArray[i]->SetPriority(currentThreadPriority);
      }
   }
}
