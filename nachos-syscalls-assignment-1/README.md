#SC_GetReg
This is a straightforward system call, we just read the value of the register
using the function machine::ReadRegister and then returned the value in register

#SC_GetPA
1. The syscall first checks for the various conditions where it had to fail,
if any of them were true then it returns -1.

2. Otherwise, we use machine::Translate to translate the provided virtual address
to a physical address.

#SC_GetPID and SC_GetPPID
1. A new static variable called pidCount and a macro MAX_THREADS was defined in
the Thread class and header file thread.h respectively.

2. The constructor of Thread Class was modified to increment pidCount and assign
this incremented pidCount to the current Thread as it's PID. (pidCount has been
initialized in such a manner that it wraps around after MAX_THREADS)

3. In the body of the syscall, the parent assigned it's PID as the PPID of the
child thread.

4. The edge case of zero PID was handled by giving the first thread PID 1 and
assinging it's parent the PID zero.

#SC_Time
1. This syscall simply returned the value of stats->totalTicks.

#SC_Yield
1. This syscall calls the Yield() function of Thread class on the currentThread.
The only caveat was to increment PC before calling Yield.

#SC_Sleep
1. Firstly, the value of the PC is incremented.

2. If the supplied time was zero, Thread::Yield() is called on the
currentThread, after disabling interrupts.

3. If the value if non-zero, then the currentThread is added in an increasing
manner to the timerQueue.

4. The timerQueue is a global variable defined in system header file and it maintains
a queue of threads waiting on the timer Interrupt. It is a sorted queue.

5. Whenever a timer Interrupt happens, the function TimerInterruptHandler is
called, in this function we check whether timerQueue is empty or not, in which
case, we return from the function.

6. Otherwise, all those threads with an expired sleep time are put on the
readyQueu using Scheduler::ReadyToRun.

#SC_Fork
1. Tobegin with, the value of PC was incremented follwing which a new Thread
Object was created.

2. The parent pointer of this new Thread was changed to point to the
currentThread, which is the creator of this forked thread.

3. Also, every parent maintains a hash map with the child pid as the key and
child_status as the value. This status is not the same as the scheduler status,
it is used to store the values CHILD_LIVE, PARENT_WAITING or the exit status of
the child, which are used in the implementation of Join.

4. initalizeChildStatus essentially sets the value of the status of the current
child as CHILD_LIVE in the hash map.

5. The new step was to create an Address Space, the constructor of AddrSpace
class was overloaded to handle the case when we fork threads. 
    
    a. In this new constructor, the Page Table mapped virtual address to physical address with a
        function virtual address + totalPagesCount. Where totalPagesCount is a global
        which stores the total number of physical pages which have been alloted as of
        yet.
    b. Next we duplicated the parent's physical memory exactly into the child's
        physical memory location.

6. Now the return value of child was changed to zero and it's state is saved
using Thread::SaveUserState.

7. The parent's return address is set to the child's pid and the child's stack
is set to the function forkStart which is defined in thread.h. This essentially
mimics the behaviour a normal thread has on returning from a sleep. This is done
to ensure that a newly forked thread behaves like any other woken thread.

8. After this the child is scheduled to run.

#SC_Join
1. First we check whether the given PID is a child of the parent by looking into
child_pids array. If it is not, we return -1.

2. If the given PID is indeed a child of the parent, we lookup it's status in the 
parent's hash map. While the status of the child is CHILD_LIVE or PARENT_WAITING, we 
send the parent to sleep and set the new status as PARENT_WAITING.

3. To obtain the child_status we use getChildStatus and to set the child status we
use setChildStatus, this is because child_status is a private variable.

#SC_Exec
1. To start out with, we have to obtain the string passed to the syscall, to do so, we
have to manually translate the addresses and obtain it just like it was done in
Print_String.

2. Once we have obtained the filename, we create an instance of OpenFile with this 
filename unless the filename being null.

3. The real change which needed to be done to get SC_Exec running was a change in
the AddrSpace constructor:
    a. The mapping of the PageTable was changed as pointed out earlier.

    b. The function bzero zeroed out the address space from the starting, it 
    was modified so that it only zeroes out the address space from the alloted 
    pages

    c. The noff function was also modified to reflect the change in the mappinf
    function.

4. The registers of the new excutable are initialized, it's state restored.

5. Finally we run the new program using Machine::Run().

#SC_Exit
1. We have some reserved exitStatus which are CHILD_LIVE and PARENT_WAITING which
cannot be used by any thread.

2. A mew static variable called ThreadCount was created in the Thread class
which store the number of live threads. This is increment in every call of 
Thread::Thread() and decremented when Finish is called.

3. If ThreadCount is 1, we halt the machine.

4. If the parent of the child is alive, we check the status of the currentThread
in the parent's child_status array, if it is PARENT_WAITING, then we wake the
child up.

5. otherwise we just set the exit code of the child in the parent's child_status
and Finish off the currentThread.
