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

    // SHORTEST JOB FIRST
    alpha = 0.5;

    // QUANTUM BASED SCHEDULING
    quantum = 0;

    // UNIX SCHEDULING
    int i;
    for(i=0; i<MAX_THREAD_COUNT; ++i) {
        cpu_count[i] = 0;
    }
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
    // Start the wait time of the thread
    thread->setStatus(READY);

    // To compute the wait time of the thread
    thread->wait_time_start = stats->totalTicks;

    // For SJF
    if(scheduler_type == 2 && thread->cpu_burst_previous >0 ){
        //Compute the burst of the thread and then sorted insert
        double estimate = alpha*thread->cpu_burst_previous 
                        + ( 1- alpha )*thread->cpu_burst_estimate;

        DEBUG('e', "\n[ pid %d ] E(n) %f E(n-1) %f burst %d\n", 
                thread->GetPID(), estimate,
                thread->cpu_burst_estimate,
                thread->cpu_burst_previous);

        // Only for non zero bursts
        if(thread->cpu_burst_previous > 0) {
            int error;
            error = thread->cpu_burst_previous - estimate;

            // This is to check for negative estimate errors
            if (error < 0 ) {
                error = error * -1;
            }

            stats->estimate_error += error;
        }

        thread->cpu_burst_estimate = estimate;

        // Add to the readyList
        readyList->SortedInsert((void *)thread, estimate);
    } else {
        readyList->Append((void *)thread);
    }

    DEBUG('t', "Putting thread \"%d\" on ready list with priority %d.\n", thread->GetPID(), thread->priority);
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
    if(scheduler_type == 0 || scheduler_type == 1) {
        return (Thread *)readyList->Remove();
    } else if ( scheduler_type >= 7 && scheduler_type <=10 ) {
        // Find the minimum
        ListElement *minPtr = readyList->first;
        if(minPtr == NULL) {
            return NULL;
        }

        int min = ((Thread *)minPtr->item)->priority;
        Thread *tempThread = (Thread *)minPtr->item;
        Thread *minThread = tempThread;
        ListElement *ptr = minPtr;

        while(ptr!=NULL) {
           tempThread = (Thread *)ptr->item;

           if(tempThread->priority <= min) {
               minPtr = ptr;
               min = tempThread->priority;
               minThread = tempThread;
           }
           ptr=ptr->next;
        }

        Thread *tempThread1;
        DEBUG('U', "\nNext to run called by %d at %d\n", currentThread->GetPID(), stats->totalTicks);
        for(ListElement *ptr1 = scheduler->readyList->first; ptr1!=NULL; ptr1=ptr1->next) {
            tempThread1 = (Thread *)ptr1->item;
            DEBUG('U', "Thread %d Priority %d\n", tempThread1->GetPID(), tempThread1->priority);
        }

        DEBUG('U', "Minimum %d\n", minThread->GetPID());

        // DELETE THE ELEMENT
        ptr = readyList->first;
        if (ptr == minPtr) {
            // corner case of the first thread
            return (Thread *)readyList->Remove();
        }

        ListElement *prev = ptr;
        ptr = ptr->next;
        while(ptr!=NULL) {
            if(ptr == minPtr) {
                prev->next = ptr->next;

                // Corner case of the last threadreadyList->
                if (ptr == readyList->last) {
                    readyList->last = prev;
                }
            }
            prev = ptr;
            ptr = ptr->next;
        }

        delete minPtr;
        return minThread;

    } else {
        int key;
        return (Thread *)readyList->SortedRemove(&key); 
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
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow
                        
    // Reset the quantum of the oldThread
    oldThread->total_time += stats->totalTicks - oldThread->start_time;

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running
  
    // This is where the cpu_burst_start of the thread starts
    nextThread->cpu_burst_start = stats->totalTicks;
    nextThread->start_time = stats->totalTicks;
    nextThread->wait_time += stats->totalTicks - nextThread->wait_time_start;

    DEBUG('t', "Switching from thread \"%d\" to thread \"%d\"\n",
	  oldThread->GetPID(), nextThread->GetPID());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    
    _SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%d\"\n", currentThread->GetPID());

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
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
        DEBUG('a', "%d", machine->ReadRegister(StackReg));
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
