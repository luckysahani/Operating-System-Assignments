// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    if(pageAlgo != NORMAL) {
        unsigned int i, size;
        int threadPid = currentThread->GetPID();

        executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        if ((noffH.noffMagic != NOFFMAGIC) && 
            (WordToHost(noffH.noffMagic) == NOFFMAGIC))
            SwapHeader(&noffH);
        ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
        size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
                + UserStackSize;	// we need to increase the size
                            // to leave room for the stack
        numPages = divRoundUp(size, PageSize);

        DEBUG('A', "Initializing address space, num pages %d, size %d, valid pages 0\n", 
                        numPages, size);
    // first, set up the translation 
        pageTable = new TranslationEntry[numPages];
        for (i = 0; i < numPages; i++) {
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = FALSE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
                                            // a separate page, we could set its 
                                            // pages to be read-only
            pageTable[i].shared= FALSE;
            pageTable[i].cached = FALSE;
            pageTable[i].threadPid = threadPid;
        }

        // Initially the number of valid pages and the number of shared pages is
        // zero
        countSharedPages = 0;
        validPages = 0;

        // Now we have to initialize the backupMemory 
        currentThread->initBackupMemory(numPages*PageSize);
    } else {
        unsigned int i, size;
        unsigned vpn, offset;
        TranslationEntry *entry;
        unsigned int pageFrame;
        int threadPid = currentThread->GetPID();

        executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        if ((noffH.noffMagic != NOFFMAGIC) && 
                (WordToHost(noffH.noffMagic) == NOFFMAGIC))
            SwapHeader(&noffH);
        ASSERT(noffH.noffMagic == NOFFMAGIC);

        // how big is address space?
        size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;	// we need to increase the size
        // to leave room for the stack
        numPages = divRoundUp(size, PageSize);
        size = numPages * PageSize;

        DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                numPages, size);

        numPagesAllocated += numPages;
        ASSERT(numPagesAllocated<= NumPhysPages);		// check we're not trying
                                                                // to run anything too big --
                                                                // at least until we have
                                                                // virtual memory

        // first, set up the translation 
        pageTable = new TranslationEntry[numPages];
        for (i = 0; i < numPages; i++) {
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = nextUnallocatedPage++;
            pageTable[i].valid = TRUE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
            // a separate page, we could set its 
            // pages to be read-only
            pageTable[i].shared= FALSE;
            pageTable[i].cached = FALSE;
            pageTable[i].threadPid = threadPid;
        }

        // zero out the entire address space, to zero the unitialized data segment 
        // and the stack segment
        bzero(&machine->mainMemory[pageTable[0].physicalPage], size);

        // then, copy in the code and data segments into memory
        if (noffH.code.size > 0) {
            DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
                    noffH.code.virtualAddr, noffH.code.size);
            vpn = noffH.code.virtualAddr/PageSize;
            offset = noffH.code.virtualAddr%PageSize;
            entry = &pageTable[vpn];
            pageFrame = entry->physicalPage;
            executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
                    noffH.code.size, noffH.code.inFileAddr);
        }
        if (noffH.initData.size > 0) {
            DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
                    noffH.initData.virtualAddr, noffH.initData.size);
            vpn = noffH.initData.virtualAddr/PageSize;
            offset = noffH.initData.virtualAddr%PageSize;
            entry = &pageTable[vpn];
            pageFrame = entry->physicalPage;
            executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
                    noffH.initData.size, noffH.initData.inFileAddr);
        }

        // Initialize the sharedPagesCount to zero
        countSharedPages = 0;
        validPages = numPages;
    }
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(AddrSpace *parentSpace, int threadPid)
{
    if(pageAlgo != NORMAL) {
        numPages = parentSpace->GetNumPages();
        countSharedPages = parentSpace->countSharedPages;
        validPages = parentSpace->validPages;
        noffH = parentSpace->noffH;
        
        // Now we copy the executable name of the parentSpace to the childSpace
        strcpy(filename, parentSpace->filename);
        unsigned i,j;

        DEBUG('a', "Initializing address space, num pages %d, shared %d, valid %d\n",
                                            numPages, countSharedPages, validPages);

        numPagesAllocated += validPages-countSharedPages;
        ASSERT(numPagesAllocated <= NumPhysPages);        // check we're not trying
                                                                                    // to run anything too big --
                                                                                    // at least until we have
                                                                                    // virtual memory

        // first, set up the translation
        TranslationEntry* parentPageTable = parentSpace->GetPageTable();
        pageTable = new TranslationEntry[numPages];
        for (i = 0; i < numPages; i++) {
            // Shared pages have to point to the correct location
            if(parentPageTable[i].shared == TRUE){
                DEBUG('A', "Linking to shared page %d\n", parentPageTable[i].physicalPage);
                pageTable[i].physicalPage = parentPageTable[i].physicalPage;
            } else {
                // Only allocate pages to those pages which are valid, the rest need
                // not be allocated pages
                if(parentPageTable[i].valid == TRUE) {
                    // If there are pages in the free pool, use them otherwise use the next
                    // unallocated page
                    int *physicalPageNumber = (int *)freedPages->Remove();
                    if(physicalPageNumber == NULL) {
                        pageTable[i].physicalPage = nextUnallocatedPage;
                        nextUnallocatedPage++;   // Update the number of pages allocated
                    } else {
                        pageTable[i].physicalPage = *physicalPageNumber;
                        delete physicalPageNumber;
                    }

                    // This stores a refernce to the pageTable entry
                    pageEntries[pageTable[i].physicalPage] = &pageTable[i];

                    DEBUG('A', "Creating a new page %d for %d copying %d\n", pageTable[i].physicalPage, 
                            currentThread->GetPID(), parentPageTable[i].physicalPage);
                } else {
                    pageTable[i].physicalPage = -1;
                }
            }

            pageTable[i].valid = parentPageTable[i].valid;
            pageTable[i].virtualPage = i;
            pageTable[i].use = parentPageTable[i].use;
            pageTable[i].dirty = parentPageTable[i].dirty;
            pageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
                                                        // a separate page, we could set its
                                                        // pages to be read-only
            pageTable[i].shared= parentPageTable[i].shared;
            pageTable[i].cached= FALSE;
            pageTable[i].threadPid = threadPid;
        }

        // Copy the contents
        unsigned startAddrParent, startAddrChild;
        for (i=0; i<numPages; i++) {
            // If the page is not shared and the page is valid then only copy
            if(!pageTable[i].shared && pageTable[i].valid){
                startAddrParent = parentPageTable[i].physicalPage * PageSize;
                startAddrChild = pageTable[i].physicalPage * PageSize;
                for(j=0; j<PageSize;++j) {
                    machine->mainMemory[startAddrChild+j] = machine->mainMemory[startAddrParent+j];
                }
            }
        }

        // Initialize the backupMemory for the child
        threadArray[threadPid]->initBackupMemory(numPages*PageSize);
    } else {
        numPages = parentSpace->GetNumPages();
        countSharedPages = parentSpace->countSharedPages;
        validPages = parentSpace->validPages;
        unsigned i,j, k;
        noffH = parentSpace->noffH;
        
        // Now we copy the executable name of the parentSpace to the childSpace
        strcpy(filename, parentSpace->filename);

        DEBUG('a', "Initializing address space, num pages %d, shared %d\n",
                numPages-countSharedPages, countSharedPages);

        numPagesAllocated += validPages-countSharedPages;
        ASSERT(numPagesAllocated <= NumPhysPages);        // check we're not trying
                                                            // to run anything too big --
                                                            // at least until we have
                                                            // virtual memory

        // first, set up the translation
        TranslationEntry* parentPageTable = parentSpace->GetPageTable();
        pageTable = new TranslationEntry[numPages];
        for (i = 0, k = 0; i < numPages; i++) {
            // Allocate a physical page only if it's not shared
            if(parentPageTable[i].shared == TRUE){
                pageTable[i].physicalPage = parentPageTable[i].physicalPage;
            } else {
                pageTable[i].physicalPage = nextUnallocatedPage++;
            }

            pageTable[i].virtualPage = i;
            pageTable[i].valid = parentPageTable[i].valid;
            pageTable[i].use = parentPageTable[i].use;
            pageTable[i].dirty = parentPageTable[i].dirty;
            pageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
                                                                    // a separate page, we could set its
                                                                    // pages to be read-only
            pageTable[i].shared= parentPageTable[i].shared;
            pageTable[i].cached= FALSE;
            pageTable[i].threadPid = threadPid;
        }

        // Copy the contents
        unsigned startAddrParent, startAddrChild;
        for (i=0; i<numPages; i++) {
            // If the current page is not shared then copy
            if(!pageTable[i].shared){
                startAddrParent = parentPageTable[i].physicalPage * PageSize;
                startAddrChild = pageTable[i].physicalPage * PageSize;
                for(j=0; j<PageSize;++j) {
                    machine->mainMemory[startAddrChild+j] = machine->mainMemory[startAddrParent+j];
                }
            }
        }
    }
}

//----------------------------------------------------------------------
// AddrSpace::createSharedPageTable(int)
// This function overwrites the page table entry of the caller with a
// new one which has a shared pages
//----------------------------------------------------------------------

unsigned 
AddrSpace::createSharedPageTable(int sharedSize, int *pagesCreated)
{
    int threadPid = currentThread->GetPID();

    // Compute the numPages in the originalSpace
    unsigned originalPages = GetNumPages();

    // Compute the number of sharedPages, round up if needed
    unsigned sharedPages = sharedSize / PageSize;
    if ( sharedSize % PageSize ) {
        sharedPages ++;
    }
    countSharedPages += sharedPages;

    // Return the number of pages created
    *pagesCreated = sharedPages;

    // Update the number of pages of the addresspace
    numPages = originalPages + sharedPages;
    unsigned i;

    // This is for NEPALI - DAKSH!!!!!!
    numPagesAllocated +=sharedPages;
    ASSERT(numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('A', "Extending address space , shared pages %d\n",
                                        sharedPages);
    // first, set up the translation
    TranslationEntry* originalPageTable = GetPageTable();
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < originalPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = originalPageTable[i].physicalPage;
        pageTable[i].valid = originalPageTable[i].valid;
        pageTable[i].use = originalPageTable[i].use;
        pageTable[i].dirty = originalPageTable[i].dirty;
        pageTable[i].readOnly = originalPageTable[i].readOnly;  	// if the code segment was entirely on
                                        			// a separate page, we could set its
                                        			// pages to be read-only
        pageTable[i].shared = originalPageTable[i].shared;
        pageTable[i].cached = originalPageTable[i].cached;
        pageTable[i].threadPid = originalPageTable[i].threadPid;

        // This stores a refernce to the pageTable entry
        pageEntries[pageTable[i].physicalPage] = &pageTable[i];
    }

    // Now set up the translation entry for the shared memory region
    for(i=originalPages; i<numPages; ++i) {
        pageTable[i].virtualPage = i;

        // If there are pages in the free pool, use them otherwise use the next
        // unallocated page
        int *physicalPageNumber = (int *)freedPages->Remove();
        if(physicalPageNumber == NULL) {
            pageTable[i].physicalPage = nextUnallocatedPage;
            bzero(&machine->mainMemory[nextUnallocatedPage*PageSize], PageSize);
            nextUnallocatedPage++;   // Update the number of pages allocated
        } else {
            pageTable[i].physicalPage = *physicalPageNumber;
            delete physicalPageNumber;
        }

        DEBUG('A', "Creating a shared page %d for %d\n", pageTable[i].physicalPage, 
                currentThread->GetPID());
        // Now store this entry into the hashMap of pageEntries
        DEBUG('R', "Adding pageEntry for %d\n", pageTable[i].physicalPage);
        pageEntries[pageTable[i].physicalPage] = &pageTable[i];

        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
        pageTable[i].shared = TRUE; // this is a shared region
        pageTable[i].cached = FALSE;
        pageTable[i].threadPid = threadPid;
    }

    // Increment the number of pages allocated by the number of shared pages
    // allocated right now
    validPages += sharedPages;

    // Set up the stuff for machine correctly
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages * PageSize;

    // free the originalPageTable
    delete originalPageTable;

    // return the starting address of the shared Page
    return originalPages * PageSize;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // When we are deleting an entire addressSpace which may be the case when we
    // are deleting the thread, we remove all mapping of the pageEntries
    int i;
    for(i=0; i<numPages; ++i) {
        if(pageTable[i].valid) {
            pageEntries[pageTable[i].physicalPage] = NULL;
            DEBUG('R', "Removing pageEntry for %d\n", pageTable[i].physicalPage);
        }
    }

    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}

TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}

//----------------------------------------------------------------------
//  AddrSpace::freePages
//  This frees the pages of the given addressSpace and adds them to the
//  freedPages list
//----------------------------------------------------------------------

void AddrSpace::freePages(bool deletePT) {
    // Run through the list of pages of the address space and add all the pages
    // into the free pages list
    int i, count;
    int *temp;

    for (i = 0, count = 0; i < numPages; i++) {
        if(pageTable[i].valid && !pageTable[i].shared) {
            count++;
            temp = new int(pageTable[i].physicalPage);
            freedPages->Append((void *)temp);
            DEBUG('A', "Freeing page %d\n", pageTable[i].physicalPage);

            // Remove the entry from the hashmap
            pageEntries[pageTable[i].physicalPage] = NULL;

            // delete this element from the PageQueue
            deleteFromPageQueue(pageTable[i].physicalPage);
        }
    }

    // delete the pageTable if yes
    if(deletePT) {
        delete pageTable;
    }

    // Reduce numPagesAllocated to match the number of pages
    numPagesAllocated -= count;
}
