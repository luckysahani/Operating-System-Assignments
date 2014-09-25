// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Semaphore:getValue
// Returns the value of the semaphore
//----------------------------------------------------------------------
int 
Semaphore::getValue() {
    return value;
}

//----------------------------------------------------------------------
// Semaphore:getValue
// Sets the value of the semaphore equal to the passed parameter
//----------------------------------------------------------------------
void 
Semaphore::setValue(int val) {
    value = val;
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {}
Lock::~Lock() {}
void Lock::Acquire() {}
void Lock::Release() {}

Condition::Condition(char* debugName) {
    queue = new List();
}

Condition::~Condition() { }
void Condition::Wait(Lock* conditionLock) { ASSERT(FALSE); }
void Condition::Signal(Lock* conditionLock) { }
void Condition::Broadcast(Lock* conditionLock) { }

// The reason why we are not disabling interrupts here is because the
// condtion variable is protected by a mutex so only one thread can exexute
// this piece of code
void
Condition::Wait(Semaphore *mutex) {
    // In a wait implementation of a CV, we have to release the mutex which is
    // passed and then the thread has to sleep, when it is woken it takes back
    // control of the mutex
    mutex->V();
	queue->Append((void *)currentThread);	// so go to sleep
                DEBUG('C', "CA\n");
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
	currentThread->Sleep();
    (void) interrupt->SetLevel(oldLevel);
    mutex->P();
}

// We are following signal and continue protocol, so essentially the signal just
// signals the condition and moves on
void 
Condition::Signal() {
    // Remove a thread and make it readyToRun
    Thread *thread = (Thread *)queue->Remove();
    if (thread != NULL)	
        scheduler->ReadyToRun(thread);
}

// Broadcast wakes up all processes in the waiting queue of condition, but
// before it can do so, it has to disable interrupts so that after the operation
// it can be in a consistent state
void
Condition::Broadcast() {
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    // Remove the threads from the queue and make them ready to run
    thread = (Thread *)queue->Remove();
    while(thread != NULL){
        scheduler->ReadyToRun(thread);
        thread = (Thread *)queue->Remove();
    }

    (void) interrupt->SetLevel(oldLevel);
}
