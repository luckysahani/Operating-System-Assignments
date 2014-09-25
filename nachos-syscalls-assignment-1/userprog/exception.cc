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

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"

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

    void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;

    // New changes
    if (!initializedConsoleSemaphores) {
        readAvail = new Semaphore("read avail", 0);
        writeDone = new Semaphore("write done", 1);
        initializedConsoleSemaphores = true;
    }

    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
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
        writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_PrintString)) {
        vaddr = machine->ReadRegister(4);
        machine->ReadMem(vaddr, 1, &memval);
        while ((*(char*)&memval) != '\0') {
            writeDone->P() ;        // wait for write to finish
            console->PutChar(*(char*)&memval);
            vaddr++;
            machine->ReadMem(vaddr, 1, &memval);
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SC_GetReg)) {
        int reg = (int) machine->ReadRegister(4);
        reg = (int) machine->ReadRegister(reg);
        machine->WriteRegister(2, reg);

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_GetPA)) {
        int virtAddress = machine->ReadRegister(4);
        unsigned vpn = (unsigned) virtAddress/PageSize;
        int physAddress;

        // Checking conditions
        if(vpn > machine->pageTableSize) {
            physAddress = -1;
        } else if (!machine->pageTable[vpn].valid) {
            physAddress = -1;
        } else if (machine->pageTable[vpn].physicalPage > NumPhysPages ){
            physAddress = -1;
        } else {
            machine->Translate(virtAddress, &physAddress, 4, FALSE);
        }
        machine->WriteRegister(2, (unsigned)physAddress);

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SC_GetPID)) {
        machine->WriteRegister(2, currentThread->getPid());

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SC_GetPPID)) {
        machine->WriteRegister(2, currentThread->getPpid());

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SC_Time)) {
        // this is a simple system call, just acces the global variable
        // totalTicks
        machine->WriteRegister(2, stats->totalTicks);

        // Advance program counters
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
    else if ((which == SyscallException) && (type == SC_Yield)) {
        // Increase the program counter before yielding
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // Call yield method on the current thread
        currentThread->Yield();
    } 
    else if ((which == SyscallException) && (type == SC_Sleep)) {
        // Increase the program counter before sleeping 
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // The time in number of ticks
        int time = machine->ReadRegister(4);

        // Yield if time is zero, or else insert the element in a sorted fashion
        // into the timerQueue
        if (time == 0) {
            IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
            currentThread->Yield();
            (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
        } else {
            timerQueue->SortedInsert((void *)currentThread, stats->totalTicks + time);  
            // Sleep the current Process
            IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
            currentThread->Sleep();
            (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
        }
    } 
    else if ((which == SyscallException) && (type == SC_Fork)) {
        // Increase the program counter before sleeping 
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        // create a new kernel thread
        Thread *child = new Thread("forked thread");

        // Set the parent of the child process
        child->parent = currentThread;

        // Add the child to the parent's list
        currentThread->initializeChildStatus(child->getPid());

        // Copy the address space of the currentThread into the child thread
        // child->space = currentThread->space;
        child->space = new AddrSpace(currentThread->space->getNumPages(), currentThread->space->getStartPhysPage()); 

        // Change the return address register to zero and save state
        machine->WriteRegister(2, 0);
        child->SaveUserState();

        // Setting the return value of the parent thread
        machine->WriteRegister(2, child->getPid());

        // Allocate the stack 
        child->StackAllocate(&forkStart, 0);

        // The child is now ready to run
        IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
        scheduler->ReadyToRun(child);
        (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
    }
    else if ((which == SyscallException) && (type == SC_Exec)) {
        // We are in the kernel space, we have to copy the name of the file 
        // by translating using ReadMem
        char filename[100];
        int i=0;

        vaddr = machine->ReadRegister(4);
        machine->ReadMem(vaddr, 1, &memval);
        while ((*(char*)&memval) != '\0') {
            filename[i]  = (char)memval;
            ++i;
            vaddr++;
            machine->ReadMem(vaddr, 1, &memval);
        }
        filename[i]  = (char)memval;

        // The above is a direct copy of StartProcess, I didn't want to change
        // its scope so it has been included here
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

        machine->Run();			// jump to the user progam
        ASSERT(FALSE);			// machine->Run never returns;
        // the address space exits
        // by doing the syscall "exit"
    }
    else if ((which == SyscallException) && (type == SC_Join)) {
        int pid = machine->ReadRegister(4);
        DEBUG('J', "Joining %d with %d\n", currentThread->getPid(), pid);

        // Search whether the child is present in the list or not,
        // after we have searched for the pid, we check if the child is live or
        // not, if it is not live then we sleep the thread or else we just
        // return the exit status of the child directly
        int childStatus = currentThread->getChildStatus(pid);

        if(childStatus!= CHILD_NOT_FOUND) {
            // The very first time, the child is live, we set the status as
            // parent waiting and send the thread to sleep, if it wakes and
            // the status is still PARENT_WAITING, we send it to sleep
            if(childStatus == CHILD_LIVE) {
                DEBUG('J', "Child %d was live: Parent %d\n", pid, currentThread->getPid());
                currentThread->setChildStatus(pid, PARENT_WAITING);

                // Send the thread to sleep
                IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
                currentThread->Sleep();
                (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
            }
            
            // The status of the thread may have changed when we come over here,
            // so we obtain it again
            childStatus = currentThread->getChildStatus(pid);
            while(childStatus == PARENT_WAITING) {
                // Sleep the thread
                DEBUG('J', "Parent %d  was sleeping: child %d\n", currentThread->getPid(), pid);

                IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
                currentThread->Sleep();
                (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

                // Obtain the new status
                childStatus = currentThread->getChildStatus(pid);
            }
        }

        // Set the return value
        DEBUG('J', "Parent %d's child %d's state %d\n", currentThread->getPid(), pid, childStatus);
        machine->WriteRegister(2, childStatus);

        // Increase the program counter after setting child Status
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SC_Exit)) {
        int exitStatus = machine->ReadRegister(4);
        DEBUG('t', "Exist Status of %d is %d\n", currentThread->getPid(), exitStatus);

        // Note here that the exit status of the child cannot be CHILD_LIVE
        // or PARENT_WAITING, in such a case we set the return status to 0
        if(exitStatus == CHILD_LIVE || exitStatus == PARENT_WAITING) {
            exitStatus = 0;
        }

        // Now the child has to set it's status to its exit status, and make it
        // ready to be destroyed, all of this must be atomic so we turn off all
        // interrupts, also we have to wake up the parent
        
        // Stop the machine if this is the only thread
        if(Thread::threadCount == 1) {
            DEBUG('t', "No more threads left, halting machine\n");
            interrupt->Halt();
        }

        // If parent is alive, signal the parent
        if(currentThread->parent != NULL) {
            // If the parent was waiting for this child thread then make it
            // ready to run
            DEBUG('c', "parent of %d exists, will kill it\n", currentThread->getPid());
            int flag = currentThread->parent->getChildStatus(currentThread->getPid());
            
            // Set the return status of the child
            currentThread->parent->setChildStatus(currentThread->getPid(), exitStatus);
           
            // If parent was waiting for the thread
            if(flag == PARENT_WAITING) {
                // The parent is now ready to run
                IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
                scheduler->ReadyToRun(currentThread->parent);
                (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
            }
        }

        // Finish the current thread
        currentThread->Finish();
    }
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
