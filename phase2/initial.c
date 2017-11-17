/* This file contains only one function i.e main() which serves as the 
 * entry point for JaeOS and performs nucleus initialization.
 */

#include "../h/const.h"
#include "../h/types.h"
#include "../e/asl.e"
#include "../e/pcb.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "/usr/include/uarm/libuarm.h"


 /* Global variables */
int procCount;
int sftBlkCount;
pcb_PTR currProc;
pcb_PTR readyQueue;
int semD[MAX_DEVICES];


extern void test(); //defined in p2test.c

void debug(int a, int b, int c, int d){
  int i = 73;
  i++;
}


/******************** MAIN FUNCTION **************************************************************/

void main() {
	state_t * newLocation;
	int i;
	int *rambase;
	int *ramsize;
	  rambase = (unsigned int)RAMBASE;
	  ramsize = (unsigned int)RAMSIZE;

	/* calculate RAMTOP */
	// Mikey G comment: Actually it is found at a "location".. use RAM_TOP
	// RAMTOP = 0x00008000 ? look at pg. 25 of uArm Principles of Operations
	// Define RAMTOP in uARMconst.h?
	unsigned int RAMTOP = *rambase + *ramsize;

	/* initiaize the PCB and ASL Lists */
	initPcbs();
	initASL();

	/* initialize nucleus maintained varibales
	i.e Process Count, Soft-block Count, Ready Queue, and Current Process */
	readyQueue = mkEmptyProcQ();
	currProc = NULL;
	procCount = 0;
	sftBlkCount = 0;

	/* initialize semaphores for each external uARM device to 0 */
	for(i = 0; i < MAX_DEVICES; ++i){
		semD[i] = 0;
	}


	/***** initialize the 4 new "areas" at a specific memory location ******/
		// areas: SysCall, Program Trap, TLB Management, and Interrupts
	/* Syscall */
	//newLocation =  (state_t *) 0x00007268 //SysCall new
	newLocation =  (state_t *) SYSBK_NEWAREA;
	newLocation -> sp = RAMTOP; 
	//newLocation -> cpsr = ALLOFF; 
	// Mikey G comment: arrow pointing to ALLOFF and says "turn on bits for sys mode & ints being
	// disabled"
	newLocation -> cpsr = ALLOFF | INTSDISABLED | STATUS_SYS_MODE; //mask all interrupts and be in kernel mode
	newLocation -> pc = (unsigned int) syscallHandler; // syscallHandler defined in exceptions.c
	newLocation -> CP15_Control = ALLOFF; //turn virtual memory off

	/* ProgramTrap */
	// newLocation =  (state_t *) 0x000071B8 //PGMT New
	newLocation = (state_t *) PGMTRAP_NEWAREA;
	newLocation -> sp = RAMTOP; 
	//newLocation -> cpsr = ALLOFF;
	newLocation -> cpsr = ALLOFF | INTSDISABLED | STATUS_SYS_MODE; //mask all interrupts and be in kernel mode
	newLocation -> pc = (unsigned int) pgmTrapHandler; // pgmTrap defined in exceptions.c
	newLocation -> CP15_Control = ALLOFF; //turn virtual memory off

	/* TLB Management */
	//newLocation =  (state_t *) 0x00007108 //TLB New
	newLocation = (state_t *) TLB_NEWAREA;
	newLocation -> sp = RAMTOP;
	//newLocation -> cpsr = ALLOFF;
	newLocation -> cpsr = ALLOFF | INTSDISABLED | STATUS_SYS_MODE; //mask all interrupts and be in kernel mode
	newLocation -> pc = (unsigned int) tlbManager; // tlbManager defined in exceptions.c
	newLocation -> CP15_Control = ALLOFF; //turn virtual memory off

	/* Interrupts */
	//newLocation =  (state_t *) 0x00007058 //Interrupt New
	newLocation = (state_t *) INT_NEWAREA;
	newLocation -> sp = RAMTOP; 
	//newLocation -> cpsr = ALLOFF; 
	newLocation -> cpsr = ALLOFF | INTSDISABLED | STATUS_SYS_MODE; //mask all interrupts and be in kernel mode
	newLocation -> pc = (unsigned int) interruptHandler; // interruptHandler defined in interrupts.c
	newLocation -> CP15_Control = ALLOFF; //turn virtual memory off


	/* create a single process ie. allocPcb and make it the current process */ 
	pcb_t *p = allocPcb();
	currProc = p;
	procCount++; // increment since we created a process

	/* initialize process state. ie. set sp to RAMTOP - FRAMESIZE, enable interrupts, kernel mode on,
		set pc to address of test, */
	p->p_s.sp = (RAMTOP - FRAME_SIZE);
	p->p_s.pc = (unsigned int) test; /* test function in p2test*/
	/* interrupts are on and is in kernel mode for test */
	p-> p_s.cpsr = ALLOFF | STATUS_SYS_MODE | INTSDISABLED;
	// ALLOFF defined in p2test.c
	// STATUS_SYS_MODE and INTSDISABLED defined in uARMTypes.h

	/* insert first process onto the readyQue */
	insertProcQ(&readyQueue, p);
	currProc = NULL;

	/* call the scheduler */
	scheduler();
	//once scheduler is called, main()'s task is completed and control will never return to main()

}

