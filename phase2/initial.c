/* This file contains only one function i.e main() which serves as the 
 * entry point for JaeOS and performs nucleus initialization.
 */

#include "../h/const.h"
#include "../h/types.h"

#include "../e/pcb.e"
#include "../e/asl.e"

#include "../e/scheduler.e"
#include "../e/exceptions.e"
#include "../e/interrupts.e"

#include "/usr/include/uarm/libuarm.h"


int processCount;
int softBlockCount;
pcb_PTR currentProcess;
pcb_PTR readyQueue;
cpu_t startTOD;
int semaphoreArray[MAXSEMA]; 
int devStatus[MAXSEMA];
int intTimerFlag;
cpu_t timeLeft;

extern void test();


int main(){
	
	/*Local Variable Declarations*/
	int i;
	pcb_PTR start = NULL;
	state_t *statePtr;
	
	devregarea_t *bus = (devregarea_t *) DEVREGAREAADDR;

	/*Set status to all interrupts disabled and system mode on*/
	setSTATUS(ALLOFF | IRQDISABLED | FIQDISABLED | SYSTEMMODE);
	
	
	/*Initialize array of semaphores to 0*/
	for (i = 0; i < MAXSEMA; i++){
		semaphoreArray[i] = 0;
		devStatus[i] = 0;
	}
	
	/*Populate the four new areas in low memory
	 *	Set the stack pointer to last page of physical
	 *	Set the pc to address of handler
	 *	Set the cpsr to
	 * 		Interrupts disabled
	 * 		Supervisor mode on
	 */
	

	statePtr = (state_t *) SYSCALLNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (unsigned int)sysCallHandler;		
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | INTSDISABLED | SYSTEMMODE;
	
	statePtr = (state_t *) PROGTRPNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (unsigned int)pgmTrapHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | INTSDISABLED | SYSTEMMODE;
								
	statePtr = (state_t *) TLBNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (unsigned int)tlbHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | INTSDISABLED | SYSTEMMODE;
	
	statePtr = (state_t *) INTERRUPTNEWADDR;
	STST(statePtr);
	statePtr->s_pc = (unsigned int)interruptHandler;
	statePtr->s_sp = bus->ramtop;
	statePtr->s_cpsr = ALLOFF | INTSDISABLED | SYSTEMMODE;



	/*Initialize PCBs and ASL*/
	initPcbs();
	initASL();
	
	/*Initialize global variables*/
	processCount = 0;
	softBlockCount = 0;
	currentProcess = NULL;
	startTOD = 0;
	readyQueue = mkEmptyProcQ();
	
	/*Allocate a starting process*/
	start = allocPcb();
	
	STST(&(start->p_s));
	start->p_s.s_pc = (memaddr) test;
	start->p_s.s_sp = bus->ramtop - (2 * PAGESIZE);
	start->p_s.s_cpsr = ALLOFF | SYSTEMMODE;
	
	/*Start the interval timer and set pseudo clock timer*/
	timeLeft = INTERVALTIME;
	intTimerFlag = FALSE;
	setTIMER(QUANTUM);
	 
	/*Increment the number of current processes*/
	processCount++;
	/*Insert the new process onto the ready queue*/
	insertProcQ(&(readyQueue), start);
	
	/*Call to the scheduler*/
	scheduler();
	
	return 1;
}
