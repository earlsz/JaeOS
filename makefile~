#$Id: Makefile,v 1.2 2004/05/01 14:53:48 morsiani Exp morsiani $
# Makefile for mipsel-linux

SUPDIR = /usr/include/uarm

DEFS = ../h/const.h ../h/types.h ../e/pcb.e ../e/asl.e ../e/initial.e ../e/interrupts.e ../e/scheduler.e ../e/exceptions.e $(SUPDIR)/libuarm.h Makefile

CFLAGS =  -mcpu=arm7tdmi -c
LDCOREFLAGS =  -T $(SUPDIR)/ldscripts/elf32ltsarm.h.uarmcore.x

CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld

#main target
all: kernel.core.uarm 

kernel.core.uarm: initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o p2test.o
	$(LD) $(LDCOREFLAGS) -o kernel.core.uarm p2test.o initial.o interrupts.o scheduler.o exceptions.o asl.o pcb.o $(SUPDIR)/libdiv.o $(SUPDIR)/crtso.o $(SUPDIR)/libuarm.o

p2test.o: p2test.c $(DEFS)
	$(CC) $(CFLAGS) p2test.c
 
initial.o: initial.c $(DEFS)
	$(CC) $(CFLAGS) initial.c

interrupts.o: interrupts.c $(DEFS)
	$(CC) $(CFLAGS) interrupts.c
 
scheduler.o: scheduler.c $(DEFS)
	$(CC) $(CFLAGS) scheduler.c

exceptions.o: exceptions.c $(DEFS)
	$(CC) $(CFLAGS) exceptions.c
 
asl.o: ../phase1/asl.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/asl.c

pcb.o: ../phase1/pcb.c $(DEFS)
	$(CC) $(CFLAGS) ../phase1/pcb.c

# crti.o: crti.s
# 	$(AS) crti.s -o crti.o


clean:
	rm -f *.o term*.umps kernel


distclean: clean
	-rm kernel.*.umps tape0.umps
