// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAX_SEMAPHORE_COUNT 1000
#define MAX_CV_COUNT 1000

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"
#include "synchop.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

extern void StartProcess (char*);
extern void StartExec(char*);

// A list of the semaphores
Semaphore *semaphores[MAX_SEMAPHORE_COUNT];
int semaphore_list[MAX_SEMAPHORE_COUNT];
int semaphore_count = 0;

// A list of the cvs 
Condition *cvs[MAX_CV_COUNT];
int cv_list[MAX_CV_COUNT];
int cv_count = 0;

void
ForkStartFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, paddr, vaddr, printval, tempval, exp;
    unsigned printvalus;	// Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);
    int exitcode;		// Used in SC_Exit
    unsigned i;
    char buffer[1024];		// Used in SC_Exec
    int waitpid;		// Used in SC_Join
    int whichChild;		// Used in SC_Join
    Thread *child;		// Used by SC_Fork
    unsigned sleeptime;		// Used by SC_Sleep
    int sharedSize; // Used by SC_ShmAllocate
    unsigned sharedMemoryStart; // Used by SC_ShmAllocate
    int key, id;  //used by SC_SemGet
    int returnValue; // used by SC_SemOp
    int adj; //used by SC_SemOp
    int op; // use by SC_SemCtl
    int sem; // used by SC_CondOp

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SC_Exit)) {
       exitcode = machine->ReadRegister(4);
       printf("[pid %d]: Exit called. Code: %d\n", currentThread->GetPID(), exitcode);
       // We do not wait for the children to finish.
       // The children will continue to run.
       // We will worry about this when and if we implement signals.
       exitThreadArray[currentThread->GetPID()] = true;

       // Find out if all threads have called exit
       for (i=0; i<thread_index; i++) {
          if (!exitThreadArray[i]) break;
       }
       currentThread->Exit(i==thread_index, exitcode);
    }
    else if ((which == SyscallException) && (type == SC_Exec)) {
       // Copy the executable name into kernel space
       vaddr = machine->ReadRegister(4);

       // There is a possibility of a PageFault and so we need to reexecute this
       // instructions until we get a valid value
       returnValue = FALSE;
       while(returnValue != TRUE) {
           returnValue = machine->ReadMem(vaddr, 1, &memval);
       }

       i = 0;
       while ((*(char*)&memval) != '\0') {
          buffer[i] = (*(char*)&memval);
          i++;
          vaddr++;

          // There is a possibility of a PageFault and so we need to reexecute this
          // instructions until we get a valid value
          returnValue = FALSE;
          while(returnValue != TRUE) {
              returnValue = machine->ReadMem(vaddr, 1, &memval);
          }
       }
       buffer[i] = (*(char*)&memval);
       StartExec(buffer);
       DEBUG('C', "%s", buffer);
    }
    else if ((which == SyscallException) && (type == SC_Join)) {
       waitpid = machine->ReadRegister(4);
       // Check if this is my child. If not, return -1.
       whichChild = currentThread->CheckIfChild (waitpid);
       if (whichChild == -1) {
          printf("[pid %d] Cannot join with non-existent child [pid %d].\n", currentThread->GetPID(), waitpid);
          machine->WriteRegister(2, -1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
       else {
          exitcode = currentThread->JoinWithChild (whichChild);
          machine->WriteRegister(2, exitcode);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
    }
    else if ((which == SyscallException) && (type == SC_Fork)) {
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       child = new Thread("Forked thread", GET_NICE_FROM_PARENT);
       child->space = new AddrSpace (currentThread->space, child->GetPID());  // Duplicates the address space
       child->SaveUserState ();		     		      // Duplicate the register set
       child->ResetReturnValue ();			     // Sets the return register to zero
       child->StackAllocate (ForkStartFunction, 0);	// Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->GetPID());		// Return value for parent
    }
    else if ((which == SyscallException) && (type == SC_Yield)) {
       currentThread->Yield();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
             writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
             writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_PrintChar)) {
        writeDone->P() ;        // wait for previous write to finish
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_PrintString)) {
       vaddr = machine->ReadRegister(4);

       // There is a possibility of a PageFault and so we need to reexecute this
       // instructions until we get a valid value
       returnValue = FALSE;
       while(returnValue != TRUE) {
           returnValue = machine->ReadMem(vaddr, 1, &memval);
       }

       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;

          // There is a possibility of a PageFault and so we need to reexecute this
          // instructions until we get a valid value
          returnValue = FALSE;
          while(returnValue != TRUE) {
              returnValue = machine->ReadMem(vaddr, 1, &memval);
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_GetReg)) {
       machine->WriteRegister(2, machine->ReadRegister(machine->ReadRegister(4))); // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_GetPA)) {
       vaddr = machine->ReadRegister(4);
       machine->WriteRegister(2, machine->GetPA(vaddr));  // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_GetPID)) {
       machine->WriteRegister(2, currentThread->GetPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_GetPPID)) {
       machine->WriteRegister(2, currentThread->GetPPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_Sleep)) {
       sleeptime = machine->ReadRegister(4);
       if (sleeptime == 0) {
          // emulate a yield
          currentThread->Yield();
       }
       else {
          currentThread->SortedInsertInWaitQueue (sleeptime+stats->totalTicks);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_Time)) {
       machine->WriteRegister(2, stats->totalTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else if ((which == SyscallException) && (type == SC_ShmAllocate)) {
       sharedSize = machine->ReadRegister(4);
       int pagesCreated;

       // create a new Page table with shared pages
       sharedMemoryStart = (unsigned)currentThread->space->createSharedPageTable(sharedSize, &pagesCreated);

       // Increment the number of page faults
       stats->numPageFaults += pagesCreated;

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

       // Return the starting address of the shared memory region
       machine->WriteRegister(2, sharedMemoryStart);
    } else if ((which == SyscallException) && (type == SC_SemGet)) {
        // Obtain the Key
        key = machine->ReadRegister(4);

        // Check if the semaphore exists, if so then just return the value
        // otherwise we will have to create a new semaphore
        id = -1;
        for( i = 0; i<semaphore_count; ++i ) {
            if( semaphore_list[i] == key ) {
                id = i;
            }
        }

        // If we have to create a new semaphore, we make sure that we disable
        // interrupts, the reason being that semaphore_count is a global
        // variable and so is the semaphores array
        // If the semaphore does not exists then create a new one
        if ( id == -1 ) {
            IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
        
            id = semaphore_count;
            semaphore_list[id] = key;
            semaphores[id] = new Semaphore("sem", 1);
            semaphore_count++;

            (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
        }
        
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // Return the id of the created Semaphore
        machine->WriteRegister(2, id);
    } else if ((which == SyscallException) && (type == SC_SemOp)) {
        id = machine->ReadRegister(4);
        adj = machine->ReadRegister(5);

        // We assusme that the semaphore implementation is a binary semaphore
        // implementation, since P and V are atomic, we needn't disable the
        // interrupts 
        if(semaphore_list[id] != -1 && id < semaphore_count) {
            if ( adj == -1 ){
                semaphores[id]->P();
            } else {
                semaphores[id]->V();
            }
        }

        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else if ((which == SyscallException) && (type == SC_SemCtl)) {
        // Obtain the passed parameters
        id = machine->ReadRegister(4);
        op = machine->ReadRegister(5);
        vaddr = machine->ReadRegister(6);
        returnValue = -1;

        // First check whether the id is valid or not
        if(semaphore_list[id] != -1 && id < semaphore_count) {
            if( op == SYNCH_REMOVE ) {
                semaphore_list[id] = -1;
                delete semaphores[id];
                returnValue = 0;
            } else if ( op == SYNCH_GET ) {
                // Translare the vaddr to a paddr and then return the value of the
                // semaphore into this address
                paddr = machine->GetPA(vaddr);
                if(paddr != -1) {
                    machine->mainMemory[paddr] = semaphores[id]->getValue();
                    returnValue = 0;
                }
            } else if ( op == SYNCH_SET ) {
                // Translate the vaddr to a paddr and then write the value stored at
                // that location in to the value of the semaphore
                paddr = machine->GetPA(vaddr);
                if(paddr != -1) {
                    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
                    semaphores[id]->setValue(machine->mainMemory[paddr]);
                    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
                    returnValue = 0;
                }
            }
        } 

        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // Return the starting address of the shared memory region
        machine->WriteRegister(2, returnValue);
    } else if ((which == SyscallException) && (type == SC_CondGet)) {
        // Obtain the Key
        key = machine->ReadRegister(4);

        // Check if the cv exists, if so then just return the value
        // otherwise we will have to create a new cv 
        id = -1;
        for( i = 0; i<cv_count; ++i ) {
            if( cv_list[i] == key ) {
                id = i;
            }
        }

        // If we have to create a new cv, we make sure that we disable
        // interrupts, the reason being that cv_count is a global
        // variable and so is the cvs array
        // If the cv does not exists then create a new one
        if ( id == -1 ) {
            IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
        
            id = cv_count;
            cv_list[id] = key;
            cvs[id] = new Condition("cv");
            cv_count++;

            (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
        }
        
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // Return the id of the created CV 
        machine->WriteRegister(2, id);
    } else if ((which == SyscallException) && (type == SC_CondOp)) {
        id = machine->ReadRegister(4);
        op = machine->ReadRegister(5);
        sem = machine->ReadRegister(6);

        // Interrupts needn't be disabled because this code is guranteed to be
        // atomic by the mutex protecting the CV
        if(cv_list[id] != -1 && semaphore_list[sem] !=-1 && sem < semaphore_count && id < cv_count) {
            if(op == COND_OP_WAIT) {
                cvs[id]->Wait(semaphores[sem]);
            } else if (op == COND_OP_SIGNAL) {
                cvs[id]->Signal();
            } else if (op == COND_OP_BROADCAST) {
                cvs[id]->Broadcast();
            }
        }

        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else if ((which == SyscallException) && (type == SC_CondRemove)) {
        id = machine->ReadRegister(4);
        returnValue = -1;

        // Interrupts needn't be disabled because this code is guranteed to be
        // atomic by the mutex protecting the CV
        if(cv_list[id] != -1 && id < cv_count) {
            cv_list[id] = -1;
            delete cvs[id];
            returnValue = 0;
        }

        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // Return the starting address of the shared memory region
        machine->WriteRegister(2, returnValue);
    } else if (which == PageFaultException)  {
        // Set the status of the thread to BLOCKED and then it goes for sleep
        // for a 1000 ticks, to model the pageFault latency
        currentThread->SortedInsertInWaitQueue (1000+stats->totalTicks);
        stats->numPageFaults++;
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
