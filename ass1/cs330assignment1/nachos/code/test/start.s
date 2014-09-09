/* Start.s 
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we don't want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main". 
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0		
	jal	sys_Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is 
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl sys_Halt
	.ent	sys_Halt
sys_Halt:
	addiu $2,$0,syscall_Halt
	syscall
	j	$31
	.end sys_Halt

	.globl sys_Exit
	.ent	sys_Exit
sys_Exit:
	addiu $2,$0,syscall_Exit
	syscall
	j	$31
	.end sys_Exit

	.globl sys_Exec
	.ent	sys_Exec
sys_Exec:
	addiu $2,$0,syscall_Exec
	syscall
	j	$31
	.end sys_Exec

	.globl sys_Join
	.ent	sys_Join
sys_Join:
	addiu $2,$0,syscall_Join
	syscall
	j	$31
	.end sys_Join

	.globl sys_Create
	.ent	sys_Create
sys_Create:
	addiu $2,$0,syscall_Create
	syscall
	j	$31
	.end sys_Create

	.globl sys_Open
	.ent	sys_Open
sys_Open:
	addiu $2,$0,syscall_Open
	syscall
	j	$31
	.end sys_Open

	.globl sys_Read
	.ent	sys_Read
sys_Read:
	addiu $2,$0,syscall_Read
	syscall
	j	$31
	.end sys_Read

	.globl sys_Write
	.ent	sys_Write
sys_Write:
	addiu $2,$0,syscall_Write
	syscall
	j	$31
	.end sys_Write

	.globl sys_Close
	.ent	sys_Close
sys_Close:
	addiu $2,$0,syscall_Close
	syscall
	j	$31
	.end sys_Close

	.globl sys_Fork
	.ent	sys_Fork
sys_Fork:
	addiu $2,$0,syscall_Fork
	syscall
	j	$31
	.end sys_Fork

	.globl sys_Yield
	.ent	sys_Yield
sys_Yield:
	addiu $2,$0,syscall_Yield
	syscall
	j	$31
	.end sys_Yield

	.globl sys_PrintInt
	.ent    sys_PrintInt
sys_PrintInt:
        addiu $2,$0,syscall_PrintInt
        syscall
        j       $31
	.end sys_PrintInt

	.globl sys_PrintChar
	.ent    sys_PrintChar
sys_PrintChar:
        addiu $2,$0,syscall_PrintChar
        syscall
        j       $31
	.end sys_PrintChar

	.globl sys_PrintString
	.ent    sys_PrintString
sys_PrintString:
        addiu $2,$0,syscall_PrintString
        syscall
        j       $31
	.end sys_PrintString

	.globl sys_GetReg
	.ent    sys_GetReg
sys_GetReg:
	addiu $2,$0,syscall_GetReg
	syscall
	j       $31
	.end sys_GetReg

	.globl sys_GetPA
	.ent    sys_GetPA
sys_GetPA:
	addiu $2,$0,syscall_GetPA
	syscall
	j       $31
	.end sys_GetPA

	.globl sys_GetPID
	.ent    sys_GetPID
sys_GetPID:
	addiu $2,$0,syscall_GetPID
	syscall
	j       $31
	.end sys_GetPID

	.globl sys_GetPPID
	.ent    sys_GetPPID
sys_GetPPID:
	addiu $2,$0,syscall_GetPPID
	syscall
	j       $31
	.end sys_GetPPID

	.globl sys_Sleep
	.ent    sys_Sleep
sys_Sleep:
	addiu $2,$0,syscall_Sleep
	syscall
	j       $31
	.end sys_Sleep

	.globl sys_GetTime
	.ent    sys_GetTime
sys_GetTime:
	addiu $2,$0,syscall_Time
	syscall
	j       $31
	.end sys_GetTime

	.globl sys_GetNumInstr
	.ent    sys_GetNumInstr
sys_GetNumInstr:
	addiu $2,$0,syscall_NumInstr
	syscall
	j	$31
	.end sys_GetNumInstr

	.globl sys_PrintIntHex
	.ent    sys_PrintIntHex
sys_PrintIntHex:
	addiu $2,$0,syscall_PrintIntHex
	syscall
	j	$31
	.end sys_PrintIntHex

/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main

