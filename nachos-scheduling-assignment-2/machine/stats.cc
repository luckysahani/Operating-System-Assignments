// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    total_cpu = 0;
    max_cpu = 0;
    min_cpu = MAX_THREAD_TIME;
    burst_count = 0;
    max_thread = 0;
    min_thread = MAX_THREAD_TIME;
    total_thread = 0;
    util = 0;
    total_wait = 0;
    avg_wait = 0.0;
    avg_thread = 0.0;
    var_thread = 0.0;
    thread_count = 0;
    square_thread = 0;
    total_completion = 0;
    max_completion = 0;
    min_completion = MAX_THREAD_TIME;
    avg_completion = 0.0;
    square_completion = 0;
    var_completion = 0;
    estimate_error = 0.0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
    // Compute the various parameters
    util = (systemTicks + userTicks)/(double)totalTicks;
    avg_thread = total_thread/(double)thread_count;
    avg_wait = total_wait/(double)thread_count;
    square_thread = square_thread/thread_count;
    var_thread = square_thread - avg_thread * avg_thread;

    avg_completion = total_completion/(double)thread_count;
    square_completion = (long long int)square_completion/(long long int)thread_count;
    var_completion = square_completion - (long long int)avg_completion * (long long int)avg_completion;
    
    printf("Ticks: total %d idle %d system %d user %d util %f\n", totalTicks, 
	idleTicks, systemTicks, userTicks, util);

    printf("For RR: %f\n", (estimate_error/(double)total_cpu)); 

    printf("\n\nCompletion statistics\n");
    printf("Total Completion time %d\n", total_completion);
    printf("Maximum Completion time %d\n", max_completion);
    printf("Minimum Completion time %d\n", min_completion);
    printf("Average Completion time %f\n", avg_completion);
    printf("Square Completion time %lld\n", square_completion);
    printf("Variance Completion time %lld\n", var_completion);

    printf("\nThread Statistics\n");
    printf("Thread Count %d\n", thread_count);
    printf("Max Thread completion time %d\n", max_thread);
    printf("Min Thread completion time %d\n", min_thread);
    printf("Avg Thread completion time %f\n", avg_thread);
    printf("Variance of Thread completion time %f\n", var_thread);
    printf("Average wait time %f\n", avg_wait);

    printf("\nCPU Usage Statistics\n");
    printf("Max CPU Burst %d\n", max_cpu);
    printf("Min CPU Burst %d\n", min_cpu);
    printf("CPU Burst count %d\n", burst_count);
    printf("Total CPU Burst time %d\n", total_cpu);
    printf("Utilization: %f\n", util);

    printf("\nTotal simulation time %d\n", totalTicks);

    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd, 
	numPacketsSent);
}
