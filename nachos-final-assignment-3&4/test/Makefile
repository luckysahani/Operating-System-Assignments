# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as
#GCCDIR = ~/gnu/local/decstation-ultrix/bin/
GCCDIR = ~/mips-i386-xgcc/bin/
LDFLAGS = -T script -N
#ASFLAGS = -mips
ASFLAGS =
CPPFLAGS = $(INCDIR)


# if you aren't cross-compiling:
#GCCDIR =
#LDFLAGS = -N -T 0
#ASFLAGS =
#CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

#CPP = /lib/cpp
CPP = /usr/bin/cpp
#CPP = $(GCCDIR)cpp
INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt shell matmult sort printtest vectorsum testregPA forkjoin testexec testyield testloop forkjoin_hard testloop1 testloop2 testloop3 testlooplong testloop4 testloop5 vmtest1 vmtest2 shm sem1 sem2 sem3 sem4 sem5_cv queue sem5_busy

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm -f strt.s

halt.o: halt.c
	$(CC) $(INCDIR) -S halt.c -o halt.s
	$(AS) $(CFLAGS) halt.s -o halt.o
#	$(CC) $(CFLAGS) -c halt.c
	rm -f halt.s
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(INCDIR) -S shell.c -o shell.s
	$(AS) $(CFLAGS) shell.s -o shell.o
#	$(CC) $(CFLAGS) -c shell.c
	rm -f shell.s
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(INCDIR) -S sort.c -o sort.s
	$(AS) $(CFLAGS) sort.s -o sort.o
#	$(CC) $(CFLAGS) -c sort.c
	rm -f sort.s
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(INCDIR) -S matmult.c -o matmult.s
	$(AS) $(CFLAGS) matmult.s -o matmult.o
#	$(CC) $(CFLAGS) -c matmult.c
	rm -f matmult.s
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

printtest.o: printtest.c
	$(CC) $(INCDIR) -S printtest.c -o printtest.s
	$(AS) $(CFLAGS) printtest.s -o printtest.o
	rm -f printtest.s
printtest: printtest.o start.o
	$(LD) $(LDFLAGS) start.o printtest.o -o printtest.coff
	../bin/coff2noff printtest.coff printtest

queue.o: queue.c
	$(CC) $(INCDIR) -S queue.c -o queue.s
	$(AS) $(CFLAGS) queue.s -o queue.o
	rm -f queue.s
queue: queue.o start.o
	$(LD) $(LDFLAGS) start.o queue.o -o queue.coff
	../bin/coff2noff queue.coff queue

sem5_busy.o: sem5_busy.c
	$(CC) $(INCDIR) -S sem5_busy.c -o sem5_busy.s
	$(AS) $(CFLAGS) sem5_busy.s -o sem5_busy.o
	rm -f sem5_busy.s
sem5_busy: sem5_busy.o start.o
	$(LD) $(LDFLAGS) start.o sem5_busy.o -o sem5_busy.coff
	../bin/coff2noff sem5_busy.coff sem5_busy

sem5_cv.o: sem5_cv.c
	$(CC) $(INCDIR) -S sem5_cv.c -o sem5_cv.s
	$(AS) $(CFLAGS) sem5_cv.s -o sem5_cv.o
	rm -f sem5_cv.s
sem5_cv: sem5_cv.o start.o
	$(LD) $(LDFLAGS) start.o sem5_cv.o -o sem5_cv.coff
	../bin/coff2noff sem5_cv.coff sem5_cv

vectorsum.o: vectorsum.c
	$(CC) $(INCDIR) -S vectorsum.c -o vectorsum.s
	$(AS) $(CFLAGS) vectorsum.s -o vectorsum.o
	rm -f vectorsum.s
vectorsum: vectorsum.o start.o
	$(LD) $(LDFLAGS) start.o vectorsum.o -o vectorsum.coff
	../bin/coff2noff vectorsum.coff vectorsum

sem1.o: sem1.c
	$(CC) $(INCDIR) -S sem1.c -o sem1.s
	$(AS) $(CFLAGS) sem1.s -o sem1.o
	rm -f sem1.s
sem1: sem1.o start.o
	$(LD) $(LDFLAGS) start.o sem1.o -o sem1.coff
	../bin/coff2noff sem1.coff sem1
	
sem2.o: sem2.c
	$(CC) $(INCDIR) -S sem2.c -o sem2.s
	$(AS) $(CFLAGS) sem2.s -o sem2.o
	rm -f sem2.s
sem2: sem2.o start.o
	$(LD) $(LDFLAGS) start.o sem2.o -o sem2.coff
	../bin/coff2noff sem2.coff sem2

sem3.o: sem3.c
	$(CC) $(INCDIR) -S sem3.c -o sem3.s
	$(AS) $(CFLAGS) sem3.s -o sem3.o
	rm -f sem3.s
sem3: sem3.o start.o
	$(LD) $(LDFLAGS) start.o sem3.o -o sem3.coff
	../bin/coff2noff sem3.coff sem3

sem4.o: sem4.c
	$(CC) $(INCDIR) -S sem4.c -o sem4.s
	$(AS) $(CFLAGS) sem4.s -o sem4.o
	rm -f sem4.s
sem4: sem4.o start.o
	$(LD) $(LDFLAGS) start.o sem4.o -o sem4.coff
	../bin/coff2noff sem4.coff sem4

shm.o: shm.c
	$(CC) $(INCDIR) -S shm.c -o shm.s
	$(AS) $(CFLAGS) shm.s -o shm.o
	rm -f shm.s
shm: shm.o start.o
	$(LD) $(LDFLAGS) start.o shm.o -o shm.coff
	../bin/coff2noff shm.coff shm
testregPA.o: testregPA.c
	$(CC) $(INCDIR) -S testregPA.c -o testregPA.s
	$(AS) $(CFLAGS) testregPA.s -o testregPA.o
	rm -f testregPA.s
testregPA: testregPA.o start.o
	$(LD) $(LDFLAGS) start.o testregPA.o -o testregPA.coff
	../bin/coff2noff testregPA.coff testregPA

forkjoin.o: forkjoin.c
	$(CC) $(INCDIR) -S forkjoin.c -o forkjoin.s
	$(AS) $(CFLAGS) forkjoin.s -o forkjoin.o
	rm -f forkjoin.s
forkjoin: forkjoin.o start.o
	$(LD) $(LDFLAGS) start.o forkjoin.o -o forkjoin.coff
	../bin/coff2noff forkjoin.coff forkjoin

testexec.o: testexec.c
	$(CC) $(INCDIR) -S testexec.c -o testexec.s
	$(AS) $(CFLAGS) testexec.s -o testexec.o
	rm -f testexec.s
testexec: testexec.o start.o
	$(LD) $(LDFLAGS) start.o testexec.o -o testexec.coff
	../bin/coff2noff testexec.coff testexec

testyield.o: testyield.c
	$(CC) $(INCDIR) -S testyield.c -o testyield.s
	$(AS) $(CFLAGS) testyield.s -o testyield.o
	rm -f testyield.s
testyield: testyield.o start.o
	$(LD) $(LDFLAGS) start.o testyield.o -o testyield.coff
	../bin/coff2noff testyield.coff testyield

testloop.o: testloop.c
	$(CC) $(INCDIR) -S testloop.c -o testloop.s
	$(AS) $(CFLAGS) testloop.s -o testloop.o
	rm -f testloop.s
testloop: testloop.o start.o
	$(LD) $(LDFLAGS) start.o testloop.o -o testloop.coff
	../bin/coff2noff testloop.coff testloop

forkjoin_hard.o: forkjoin_hard.c
	$(CC) $(INCDIR) -S forkjoin_hard.c -o forkjoin_hard.s
	$(AS) $(CFLAGS) forkjoin_hard.s -o forkjoin_hard.o
	rm -f forkjoin_hard.s
forkjoin_hard: forkjoin_hard.o start.o
	$(LD) $(LDFLAGS) start.o forkjoin_hard.o -o forkjoin_hard.coff
	../bin/coff2noff forkjoin_hard.coff forkjoin_hard

testloop1.o: testloop1.c
	$(CC) $(INCDIR) -S testloop1.c -o testloop1.s
	$(AS) $(CFLAGS) testloop1.s -o testloop1.o
	rm -f testloop1.s
testloop1: testloop1.o start.o
	$(LD) $(LDFLAGS) start.o testloop1.o -o testloop1.coff
	../bin/coff2noff testloop1.coff testloop1

testloop2.o: testloop2.c
	$(CC) $(INCDIR) -S testloop2.c -o testloop2.s
	$(AS) $(CFLAGS) testloop2.s -o testloop2.o
	rm -f testloop2.s
testloop2: testloop2.o start.o
	$(LD) $(LDFLAGS) start.o testloop2.o -o testloop2.coff
	../bin/coff2noff testloop2.coff testloop2

testloop3.o: testloop3.c
	$(CC) $(INCDIR) -S testloop3.c -o testloop3.s
	$(AS) $(CFLAGS) testloop3.s -o testloop3.o
	rm -f testloop3.s
testloop3: testloop3.o start.o
	$(LD) $(LDFLAGS) start.o testloop3.o -o testloop3.coff
	../bin/coff2noff testloop3.coff testloop3

testlooplong.o: testlooplong.c
	$(CC) $(INCDIR) -S testlooplong.c -o testlooplong.s
	$(AS) $(CFLAGS) testlooplong.s -o testlooplong.o
	rm -f testlooplong.s
testlooplong: testlooplong.o start.o
	$(LD) $(LDFLAGS) start.o testlooplong.o -o testlooplong.coff
	../bin/coff2noff testlooplong.coff testlooplong

testloop4.o: testloop4.c
	$(CC) $(INCDIR) -S testloop4.c -o testloop4.s
	$(AS) $(CFLAGS) testloop4.s -o testloop4.o
	rm -f testloop4.s
testloop4: testloop4.o start.o
	$(LD) $(LDFLAGS) start.o testloop4.o -o testloop4.coff
	../bin/coff2noff testloop4.coff testloop4

testloop5.o: testloop5.c
	$(CC) $(INCDIR) -S testloop5.c -o testloop5.s
	$(AS) $(CFLAGS) testloop5.s -o testloop5.o
	rm -f testloop5.s
testloop5: testloop5.o start.o
	$(LD) $(LDFLAGS) start.o testloop5.o -o testloop5.coff
	../bin/coff2noff testloop5.coff testloop5

vmtest1.o: vmtest1.c
	$(CC) $(INCDIR) -S vmtest1.c -o vmtest1.s
	$(AS) $(CFLAGS) vmtest1.s -o vmtest1.o
	rm -f vmtest1.s
vmtest1: vmtest1.o start.o
	$(LD) $(LDFLAGS) start.o vmtest1.o -o vmtest1.coff
	../bin/coff2noff vmtest1.coff vmtest1

vmtest2.o: vmtest2.c
	$(CC) $(INCDIR) -S vmtest2.c -o vmtest2.s
	$(AS) $(CFLAGS) vmtest2.s -o vmtest2.o
	rm -f vmtest2.s
vmtest2: vmtest2.o start.o
	$(LD) $(LDFLAGS) start.o vmtest2.o -o vmtest2.coff
	../bin/coff2noff vmtest2.coff vmtest2

clean:
	rm -f start.o halt.o halt shell.o shell sort.o sort matmult.o matmult halt.coff shell.coff sort.coff matmult.coff printtest.o printtest printtest.coff vectorsum.o vectorsum.coff vectorsum testregPA.o testregPA.coff testregPA forkjoin.o forkjoin.coff forkjoin testexec.o testexec.coff testexec testyield.o testyield.coff testyield testloop.o testloop.coff testloop forkjoin_hard.o forkjoin_hard.coff forkjoin_hard testloop1.o testloop1.coff testloop1 testloop2.o testloop2.coff testloop2 testloop3.o testloop3.coff testloop3 testlooplong.o testlooplong.coff testlooplong testloop4.o testloop4 testloop4.coff testloop5.o testloop5 testloop5.coff vmtest1.o vmtest1 vmtest1.coff vmtest2 vmtest2.o vmtest2.coff
