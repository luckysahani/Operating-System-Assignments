// translate.cc 
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception. 
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <stdlib.h>
#include "copyright.h"
#include "machine.h"
#include "addrspace.h"
#include "system.h"
#include "filesys.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }

extern int* getLRUClockFrame();
extern void printQueue();

//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into 
//	the location pointed to by "value".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------

bool
Machine::ReadMem(int addr, int size, int *value)
{
    int data;
    ExceptionType exception;
    int physicalAddress;
    
    DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);
    
    exception = Translate(addr, &physicalAddress, size, FALSE);
    if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	data = machine->mainMemory[physicalAddress];
	*value = data;
	break;
	
      case 2:
	data = *(unsigned short *) &machine->mainMemory[physicalAddress];
	*value = ShortToHost(data);
	break;
	
      case 4:
	data = *(unsigned int *) &machine->mainMemory[physicalAddress];
	*value = WordToHost(data);
	break;

      default: ASSERT(FALSE);
    }
    
    DEBUG('a', "\tvalue read = %8.8x\n", *value);
    return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool
Machine::WriteMem(int addr, int size, int value)
{
    ExceptionType exception;
    int physicalAddress;
     
    DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

    exception = Translate(addr, &physicalAddress, size, TRUE);
    if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	machine->mainMemory[physicalAddress] = (unsigned char) (value & 0xff);
	break;

      case 2:
	*(unsigned short *) &machine->mainMemory[physicalAddress]
		= ShortToMachine((unsigned short) (value & 0xffff));
	break;
      
      case 4:
	*(unsigned int *) &machine->mainMemory[physicalAddress]
		= WordToMachine((unsigned int) value);
	break;
	
      default: ASSERT(FALSE);
    }
    
    return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using 
//	either a page table or a TLB.  Check for alignment and all sorts 
//	of other errors, and if everything is ok, set the use/dirty bits in 
//	the translation table entry, and store the translated physical 
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if TRUE, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing)
{
    int i, j;
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;
    int flag = 0;
    unsigned int numPages = currentThread->space->GetNumPages();
    unsigned int StartMachine, StartBackup;

    DEBUG('a', "\tTranslate 0x%x, %s: \n\t", virtAddr, writing ? "write" : "read");

    // check for alignment errors
    if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))){
        DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
        return AddressErrorException;
    }

    // we must have either a TLB or a page table, but not both!
    ASSERT(tlb == NULL || pageTable == NULL);	
    ASSERT(tlb != NULL || pageTable != NULL);	

    // calculate the virtual page number, and offset within the page,
    // from the virtual address
    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;

    if (tlb == NULL) {		// => page table => vpn is index into table
        if (vpn >= pageTableSize) {
            DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
                    virtAddr, pageTableSize);
            return AddressErrorException;
        } else if (!pageTable[vpn].valid) {
            if(currentThread->space->validPages < numPages && pageAlgo != NORMAL) {
                // In this case we have to perform demand paging whose code is
                // given below
                flag = 1;
            } else {
                DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
                        virtAddr, pageTableSize);
                return PageFaultException;
            }
        }
        entry = &pageTable[vpn];

        // Demand Paging
        if(flag) {
            unsigned int size = numPages * PageSize;
            unsigned int readSize = PageSize;

            if(numPagesAllocated == NumPhysPages) {
                // here we have to handle page replacement
                DEBUG('R', "\nInvoking page replacement algorithm: ");

                // The frame which will be replace
                int *frameToReplace;
                TranslationEntry *frameEntry;

                // Select the frame according to the algorithm
                if(pageAlgo == FIFO) {
                    frameToReplace = (int *)pageQueue->Remove();
                    DEBUG('R', "\n\tFIFO selects frame %d", *frameToReplace);
                } else if (pageAlgo == RANDOM) {
                    frameToReplace = new int(rand()%(NumPhysPages-1));
                    DEBUG('R', "\n\tRANDOM selects frame %d", *frameToReplace);
                } else if (pageAlgo == LRU) {
                    frameToReplace = (int *)pageQueue->Remove();
                    DEBUG('R', "\n\tLRU selects frame %d", *frameToReplace);
                } else if (pageAlgo == LRU_CLOCK) {
                    frameToReplace = getLRUClockFrame();
                    DEBUG('R', "\n\tLRU CLOCK selects frame %d", *frameToReplace);
                }

                // Get the PTE of this frame
                frameEntry = pageEntries[*frameToReplace];
                Thread *thread = threadArray[frameEntry->threadPid];

                // We have to delete the frameToReplace
                delete frameToReplace;

                DEBUG('R', "\n\tvirtual %d physical %d thread %d shared %d valid %d", 
                        frameEntry->virtualPage, frameEntry->physicalPage, 
                        frameEntry->threadPid, frameEntry->shared,
                        frameEntry->valid);

                // Now this page should no longer be valid
                frameEntry->valid = FALSE;
                thread->space->validPages--;

                // Now we have to copy the values between the backupMemory and
                // machineMemory in case the page is dirty
                if(frameEntry->dirty) {
                    DEBUG('R', "\n\tpage %d of thread %d is dirty"
                            , frameEntry->virtualPage, frameEntry->threadPid);

                    // Now this frameEntry is cached
                    frameEntry->cached = TRUE;

                    // Now we have to copy the mainMemory to backupMemory
                    StartMachine = frameEntry->physicalPage * PageSize;
                    StartBackup = frameEntry->virtualPage * PageSize;
                    for(j=0; j<PageSize; j++) {
                        thread->backupMemory[StartBackup+j] = machine->mainMemory[StartMachine+j];
                        DEBUG('r', "B: %d\n", thread->backupMemory[StartBackup+j]);
                        DEBUG('r', "M: %d\n", machine->mainMemory[StartMachine+j]);
                    }
                }

                // Now we have to change the pageframe of entry
                entry->physicalPage = frameEntry->physicalPage;
                DEBUG('R', "\n\n");
            } else {
                // Increment the numPagesAllocated
                numPagesAllocated++;

                // We either take the page from the pool of freed pages of we take a
                // page from the pool of unallocated pages
                int *physicalPageNumber = (int *)freedPages->Remove();
                if(physicalPageNumber == NULL) {
                    entry->physicalPage = nextUnallocatedPage;
                    nextUnallocatedPage++;   // Update the number of pages allocated
                } else {
                    entry->physicalPage = *physicalPageNumber;
                    delete physicalPageNumber;
                }
            }

            // This stores a refernce to the pageTable entry
            pageEntries[entry->physicalPage] = entry;

            // This is for ease of access
            pageFrame = entry->physicalPage;

            DEBUG('A', "Allocating physical page %d VPN %d virtualaddress %d\n", pageFrame, vpn, virtAddr);

            // In case of FIFO or LRU_CLOCK we maintain a fifo queue of elements
            if(pageAlgo == FIFO) {
                int *temp = new int(pageFrame);
                pageQueue->Append((void *)temp);
                DEBUG('Q', "FIFO Update ");
                printQueue();
            }

            if(pageAlgo == LRU_CLOCK){
                deleteFromPageQueue(pageFrame);
                int *temp = new int(pageFrame);
                pageQueue->Append((void *)temp);

                // If this is the first page fault then we set the LRU_CLOCK
                // hand
                if(stats->numPageFaults == 0) {
                    DEBUG('q', "Setting the LRU_CLOCK Hand to %d\n", *temp);
                    LRUClockhand = new int(*temp);
                }

                DEBUG('Q', "LRU_CLK List \t ");
                printQueue();
            }

            // zero out this particular page
            bzero(&machine->mainMemory[pageFrame*PageSize], PageSize);

            // Now here are two cases, we may either have to use the
            // backupMemory of the thread or the mainMemory
            if(entry->cached) {
                DEBUG('R', "page %d of %d has been modified\n", 
                        entry->virtualPage, entry->threadPid);

                StartMachine = pageFrame * PageSize;
                StartBackup = entry->virtualPage * PageSize;
                for(j=0; j<PageSize; ++j) {
                    machine->mainMemory[StartMachine+j] = currentThread->backupMemory[StartBackup+j];
                }
            } else {
                // Now copy the corresponding area from memory
                if( vpn == (numPages - 1) ) {
                    readSize = size - vpn * PageSize;
                }

                executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize]),
                        readSize, noffH.code.inFileAddr + vpn*PageSize);
            }
                executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize]),
                        readSize, noffH.code.inFileAddr + vpn*PageSize);

                // delete the opened executable
                delete executable;
            }

            // The number of valid pages of this thread has increased
            currentThread->space->validPages++;

            // Mark this pagetable entry as valid
            entry->valid = TRUE;

            // Now store this entry into the hashMap of pageEntries
            DEBUG('R', "Adding pageEntry for %d\n", entry->physicalPage);
            pageEntries[entry->physicalPage] = entry;

            return PageFaultException;
        }
    } else {
        for (entry = NULL, i = 0; i < TLBSize; i++)
            if (tlb[i].valid && (tlb[i].virtualPage == vpn)) {
                entry = &tlb[i];			// FOUND!
                break;
            }
        if (entry == NULL) {				// not found
            DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
            return PageFaultException;		// really, this is a TLB fault,
            // the page may be in memory,
            // but not in the TLB
        }
    }

    if (entry->readOnly && writing) {	// trying to write to a read-only page
        DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
        return ReadOnlyException;
    }
    pageFrame = entry->physicalPage;

    // if the pageFrame is too big, there is something really wrong! 
    // An invalid translation was loaded into the page table or TLB. 
    if (pageFrame >= NumPhysPages) { 
        DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
        return BusErrorException;
    }
    entry->use = TRUE;		// set the use, dirty bits
    if (writing)
        entry->dirty = TRUE;
    *physAddr = pageFrame * PageSize + offset;
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG('a', "phys addr = 0x%x\n", *physAddr);

    // A new frame is updated so ww change the LRU pageQueue
    if(pageAlgo == LRU) {
        // First of all we delete the element if it's in the queue
        deleteFromPageQueue(pageFrame);

        // We add this element to the bottom of the pageQueue, every time we
        // pop, we pop from the head so this is the same as LRU
        int *temp = new int(pageFrame);
        pageQueue->Append((void *)temp);
        DEBUG('Q', "LRU Update %d\t", pageFrame);
        printQueue();
    } else if (pageAlgo == LRU_CLOCK) {
        // We have to set the reference bit of the element if it's not set and
        // let it remain the same if it's set
        referenceBit[pageFrame] = 1;
        DEBUG('Q', "LRU_C Update %d\t", pageFrame);
        printQueue();
    }

    return NoException;
}

//----------------------------------------------------------------------
// Machine::GetPA
//      Returns the physical address corresponding to the passed virtual
//      address. Error conditions to check: virtual page number (vpn) is
//      bigger than the page table size and the page table entry is not valid.
//      In case of error, returns -1.
//----------------------------------------------------------------------

int
Machine::GetPA (unsigned vaddr)
{
   unsigned vpn = vaddr/PageSize;
   unsigned offset = vaddr % PageSize;
   TranslationEntry *entry;
   unsigned int pageFrame;

   if ((vpn < pageTableSize) && pageTable[vpn].valid) {
      entry = &pageTable[vpn];
      pageFrame = entry->physicalPage;
      if (pageFrame >= NumPhysPages) return -1;
      return pageFrame * PageSize + offset;
   }
   else return -1;
}
