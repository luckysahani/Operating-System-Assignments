/* syscalls.h 
 * 	Nachos system call interface.  These are Nachos kernel operations
 * 	that can be invoked from user programs, by trapping to the kernel
 *	via the "syscall" instruction.
 *
 *	This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define syscall_Halt		0
#define syscall_Exit		1
#define syscall_Exec		2
#define syscall_Join		3
#define syscall_Create		4
#define syscall_Open		5
#define syscall_Read		6
#define syscall_Write		7
#define syscall_Close		8
#define syscall_Fork		9
#define syscall_Yield		10

// New syscalls defined by Mainak

#define syscall_PrintInt	11
#define syscall_PrintChar	12
#define syscall_PrintString	13

#define syscall_GetReg		14
#define syscall_GetPA		15
#define syscall_GetPID		16
#define syscall_GetPPID		17

#define syscall_Sleep		18

#define syscall_Time		19

#define syscall_PrintIntHex  	20

#define syscall_NumInstr	50

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void sys_Halt();		
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void sys_Exit(int status);	

/* A unique identifier for an executing user program (address space) */
/* This is same as PID. */
typedef int SpaceId;	
 
/* Run the executable, stored in the Nachos file "name"
 */
void sys_Exec(char *name);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int sys_Join(SpaceId id); 	
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;	

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with "name" */
void sys_Create(char *name);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId sys_Open(char *name);

/* Write "size" bytes from "buffer" to the open file. */
void sys_Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int sys_Read(char *buffer, int size, OpenFileId id);

/* Close the file, we're done reading and writing to it. */
void sys_Close(OpenFileId id);



/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 */

/* Fork a thread. Returns child pid to parent and zero to child.
 */
int sys_Fork(void);

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void sys_Yield();		

// New definitions

void sys_PrintInt (int x);

void sys_PrintChar (char x);

void sys_PrintString (char *x);

void sys_PrintIntHex (int x);

int sys_GetReg (int regno);

int sys_GetPA (unsigned vaddr);

int sys_GetPID (void);

int sys_GetPPID (void);

void sys_Sleep (unsigned);

int sys_GetTime (void);

int sys_GetNumInstr (void);

#endif /* IN_ASM */

#endif /* SYSCALL_H */
