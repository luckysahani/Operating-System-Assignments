// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

Timer *timer; // The hardware timer to be used

//----------------------------------------------------------------------
// PreemptHandler
// 	Interrupt handler for the timer device.  The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
static void
PreemptHandler(int dummy)
{
    DEBUG('T', "\nTimer Interrupt at %d\n", stats->totalTicks);

    TimeSortedWaitQueue *ptr;
    if (interrupt->getStatus() != IdleMode) {
        // Check the head of the sleep queue
        while ((sleepQueueHead != NULL) && (sleepQueueHead->GetWhen() <= (unsigned)stats->totalTicks)) {
           sleepQueueHead->GetThread()->Schedule();
           ptr = sleepQueueHead;
           sleepQueueHead = sleepQueueHead->GetNext();
           delete ptr;
        }
        //printf("[%d] Timer interrupt.\n", stats->totalTicks);
    }

    // Preempt only for the algorithms greater than 3
    if(scheduler->scheduler_type >=3) {
        currentThread->timerYield = true;
        interrupt->YieldOnReturn();
    }
} 

//----------------------------------------------------------------------
// RunBatchProcess 
// The function run when a batch thread gets scheduled for the very first time
//----------------------------------------------------------------------
void
BatchStartFunction(int dummy)
{
    currentThread->Startup();
    DEBUG('t', "Running thread \"%d\" for the first time\n", currentThread->GetPID());
    // Call the start_time function over her
    machine->Run();
}

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    space = new AddrSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    // This is when this thread gets started for the first time
    currentThread->start_time = stats->totalTicks;

    // Start the timer
    timer = new Timer(PreemptHandler, 0, false , 100);

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
    // the address space exits
    // by doing the syscall "exit"
}

//----------------------------------------------------------------------
// RunBatchProcess 
// Run a batch of processes
//----------------------------------------------------------------------

void
RunBatchProcess(char *filename) {
    // Open the given executable
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }

    // Compute the length of the file and then read that much from the file
    int filelength = executable->Length();
    char readbuffer[filelength];
    OpenFile *programfile;
    Thread *thread;

    // Read the executable
    executable->ReadAt(readbuffer, (filelength-1), 0);
    DEBUG('s', "running batch jobs from \"%s\"\n", filename);
   
    // Create threads and enque them
    int i=0, k=0;
    char name[100];
    char priority[10];

    // First of all read the type of scheduling algorithm
    while(readbuffer[i]!='\n') {
        name[k]=readbuffer[i];
        ++i; ++k;
    }
    name[k]='\0';
    k=0; ++i;
    scheduler->scheduler_type = atoi(name);

    DEBUG('s', "Scheduling algorithm is \"%d\"\n", scheduler->scheduler_type);

    // Read the names of the different programs and create a thread for each of
    // them, then yield the currently executing thread
    while(i<filelength){
        // If no priority is supplied by a thread then we assign a default priority of 100
        if(readbuffer[i]=='\n') {
            name[k]='\0';
            k=0; ++i;

            // Creating a new thread and enquing it
            programfile = fileSystem->Open(name);
            ASSERT(programfile != NULL);
            thread = new Thread(name, 100, true);
            thread->space = new AddrSpace(programfile);  // Duplicates the address space
            thread->space->InitRegisters();		// set the initial register values
            thread->SaveUserState();		// load page table register
            thread->StackAllocate(BatchStartFunction, 0);	// Make it ready for a later context switch
            thread->Schedule();
            delete programfile;			// close file
        } else {
            // This is the normal case when priority has been suplied
            if(readbuffer[i]!=' '){
                name[k]=readbuffer[i];
                ++i; ++k;
            } else {
                name[k]='\0';
                k=0; ++i;
                while(readbuffer[i]!='\n') {
                    priority[k]=readbuffer[i];
                    ++i; ++k;
                }
                priority[k]='\0';
                k=0; ++i;

                // Creating a new thread and enquing it
                programfile = fileSystem->Open(name);
                ASSERT(programfile != NULL);
                thread = new Thread(name, atoi(priority), true);
                thread->space = new AddrSpace(programfile);  // Duplicates the address space
                thread->space->InitRegisters();		// set the initial register values
                thread->SaveUserState();		// load page table register
                thread->StackAllocate(BatchStartFunction, 0);	// Make it ready for a later context switch
                thread->Schedule();
                delete programfile;			// close file
            }
        }
    }

    // Setting up the hardware timer
    int quantum;
    switch(scheduler->scheduler_type) {
        case 1: 
        case 2:
                quantum = 100;
                break;
        case 3:
                quantum = 120;
                break;
        case 4: 
                quantum = 60;
                break;
        case 5: 
                quantum = 30;
                break;
        case 6: 
                quantum = 20;
                break;
        case 7: 
                quantum = 120;
                break;
        case 8: 
                quantum = 60;
                break;
        case 9: 
                quantum = 30;
                break;
        case 10: 
                quantum = 20;
                break;
        default: 
                break;
    }

    // Set up the timer Interrupt
    timer = new Timer(PreemptHandler, 0, false, quantum);

    // The main thread exits after this
    currentThread->Exit(false, 0);
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);

    for (;;) {
        readAvail->P();		// wait for character to arrive
        ch = console->GetChar();
        console->PutChar(ch);	// echo it!
        writeDone->P() ;        // wait for write to finish
        if (ch == 'q') return;  // if q, quit
    }
}
