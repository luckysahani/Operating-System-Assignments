// thread.h 
//	Data structures for managing threads.  A thread represents
//	sequential execution of code within a program.
//	So the state of a thread includes the program counter,
//	the processor registers, and the execution stack.
//	
// 	Note that because we allocate a fixed size stack for each
//	thread, it is possible to overflow the stack -- for instance,
//	by recursing to too deep a level.  The most common reason
//	for this occuring is allocating large data structures
//	on the stack.  For instance, this will cause problems:
//
//		void foo() { int buf[1000]; ...}
//
//	Instead, you should allocate all data structures dynamically:
//
//		void foo() { int *buf = new int[1000]; ...}
//
//
// 	Bad things happen if you overflow the stack, and in the worst 
//	case, the problem may not be caught explicitly.  Instead,
//	the only symptom may be bizarre segmentation faults.  (Of course,
//	other problems can cause seg faults, so that isn't a sure sign
//	that your thread stacks are too small.)
//	
//	One thing to try if you find yourself with seg faults is to
//	increase the size of thread stack -- ThreadStackSize.
//
//  	In this interface, forking a thread takes two steps.
//	We must first allocate a data structure for it: "t = new Thread".
//	Only then can we do the fork: "t->fork(f, arg)".
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

// CPU register state to be saved on context switch.  
// The SPARC and MIPS only need 10 registers, but the Snake needs 18.
// For simplicity, this is just the max over all architectures.
#define MachineStateSize 18 

// This is under the assumption that the children always return a positive
// return value
#define PARENT_WAITING -1
#define CHILD_LIVE -2
#define CHILD_NOT_FOUND -1
#define MAX_THREADS 10000

// Size of the thread's private execution stack.
// WATCH OUT IF THIS ISN'T BIG ENOUGH!!!!!
#define StackSize	(4 * 1024)	// in words

// Thread state
enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED };

// external function, dummy routine whose sole job is to call Thread::Print
extern void ThreadPrint(int arg);	 

extern void forkStart(int arg);            // the function which is called when a process is forked 
// The following class defines a "thread control block" -- which
// represents a single thread of execution.
//
//  Every thread has:
//     an execution stack for activation records ("stackTop" and "stack")
//     space to save CPU registers while not running ("machineState")
//     a "status" (running/ready/blocked)
//    
//  Some threads also belong to a user address space; threads
//  that only run in the kernel have a NULL address space.

class Thread {
  private:
    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for SWITCH to work.
    int* stackTop;			 // the current stack pointer
    int machineState[MachineStateSize];  // all registers except for stackTop

  public:
    static int pidCount;            // Maintain a count of pids
    static int threadCount;         // Maintains a count of total threads

    Thread(char* debugName);		// initialize a Thread 
    ~Thread(); 				// deallocate a Thread
					// NOTE -- thread being deleted
					// must not be running when delete 
					// is called

    // basic thread operations

    void Fork(VoidFunctionPtr func, int arg); 	// Make thread run (*func)(arg)
    void Yield();  				// Relinquish the CPU if any 
						// other thread is runnable
    void Sleep();  				// Put the thread to sleep and 
						// relinquish the processor
    void Finish();  				// The thread is done executing
    
    void CheckOverflow();   			// Check if thread has 
						// overflowed its stack
    void setStatus(ThreadStatus st) { status = st; }
    char* getName() { return (name); }
    void Print() { printf("%s, ", name); }
   
    // A public pointer the parent 
    Thread *parent;

    // To store the child pid
    int *child_pids;

    // Return the threads pid
    int getPid();
    int getPpid();

    void StackAllocate(VoidFunctionPtr func, int arg);
    					// Allocate a stack for thread.
					// Used internally by Fork()
  private:
    // To store the state of the children
    int *child_status;

    // some of the private data for this class is listed above
    int childCount;         // To store the number of children of the thread 
    int* stack; 	 		// Bottom of the stack 
					// NULL if this is the main thread
					// (If NULL, don't deallocate stack)
    ThreadStatus status;		// ready, running or blocked
    char* name;

    int pid, ppid;			// My pid and my parent's pid

#ifdef USER_PROGRAM
// A thread running a user program actually has *two* sets of CPU registers -- 
// one for its state while executing user code, one for its state 
// while executing kernel code.

    int userRegisters[NumTotalRegs];	// user-level CPU register state
    
  public:
    void SaveUserState();		// save user-level register state
    void RestoreUserState();		// restore user-level register state
    AddrSpace *space;			// User code this thread is running.

    // To manipulate the state of the child state
    int searchChildPid(int child_pid);
    void initializeChildStatus(int child_pid); 
    int getChildStatus(int child_pid);
    void setChildStatus(int child_pid, int status);

    // To maintain the child counts
    void incrementChildCount();     // Increments the count of the number of variables
    void decrementChildCount();     // Decrement the count of the number of variables

#endif
};

// Magical machine-dependent routines, defined in switch.s

extern "C" {
// First frame on thread execution stack; 
//   	enable interrupts
//	call "func"
//	(when func returns, if ever) call ThreadFinish()
void _ThreadRoot();

// Stop running oldThread and start running newThread
void _SWITCH(Thread *oldThread, Thread *newThread);
}

#endif // THREAD_H
